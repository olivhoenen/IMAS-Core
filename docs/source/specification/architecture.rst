Architecture overview
=====================

This page gives the design context needed to read the requirements. It
describes the structure of IMAS-Core as built at the baseline; it sets no
requirements of its own.

Layers
------

A call from a high-level interface goes through four layers:

.. code-block:: text

   High-level interface (IMAS-Python, Fortran/C++/Java/MATLAB HLIs)
           |
           |  imas_core Python package (_al_lowlevel.pyx)  or  C API
           v
   +---------------------------------------------------------------+
   | HLI layer:  al_begin_global_action, al_read_data, ...         |
   |   - strips a trailing "/0" from IDS names                     |
   |   - dispatches to bound plugins, or skips empty writes        |
   +---------------------------------------------------------------+
           |                                   |
           v                                   v
   +-------------------------------+   +---------------------------+
   | Low-level layer: al_plugin_*  |   | Plugins (shared objects   |
   |   - handle table              |<--|  loaded with dlopen); may |
   |   - context objects           |   |  re-enter al_plugin_*     |
   |   - version checks            |   +---------------------------+
   |   - type conversion on read   |
   +-------------------------------+
           |
           v
   +---------------------------------------------------------------+
   | Backend (one object per data entry), chosen from the URI:     |
   |   HDF5 | MDSplus | UDA | Memory | ASCII | Flexbuffers | None  |
   +---------------------------------------------------------------+

- The **HLI layer** (``al_begin_global_action``, ``al_begin_slice_action``,
  ``al_begin_timerange_action``, ``al_begin_arraystruct_action``,
  ``al_read_data``, ``al_write_data``, ``al_end_action``) is what language
  bindings call. It looks up the plugins bound to the path and, when there
  are none, calls the low-level layer. See :doc:`plugins`.
- The **low-level layer** (the ``al_plugin_*`` functions, plus
  ``al_begin_dataentry_action``, ``al_close_pulse``, ``al_delete_data``,
  ``al_iterate_over_arraystruct``, ``al_get_occurrences`` and
  ``al_list_filled_paths``) owns the handle table and the context objects
  and calls the backend. Plugins call this layer directly to reach the
  backend without being dispatched again.
- The **backend** implements the abstract class ``Backend``
  (``include/al_backend.h``). ``Backend::initBackend``
  (``src/al_backend.cpp``) creates it when a data entry is opened. A
  backend that was not compiled in is refused there.

Every C function returns an ``al_status_t`` (a code and a 256-byte
message). The C++ code raises exceptions internally and converts them to
status codes at the C boundary. See :doc:`errors`.

Contexts and handles
--------------------

IMAS-Core keeps three kinds of context objects (``include/al_context.h``).
The caller only sees integer handles that index a process-wide table
(``Lowlevel::llenvStore`` in ``src/al_lowlevel.cpp``). Each table entry pairs
a context with the backend object of its data entry.

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - Context
     - Type id
     - Created by, and content
   * - ``DataEntryContext``
     - ``CTX_PULSE_TYPE`` (101)
     - ``al_begin_dataentry_action``. Holds the parsed URI and the backend
       id. Owns the backend object.
   * - ``OperationContext``
     - ``CTX_OPERATION_TYPE`` (102)
     - ``al_begin_global_action``, ``al_begin_slice_action``,
       ``al_begin_timerange_action``. Holds the IDS name and occurrence,
       access mode (read or write), range mode (global, slice or time
       range), time and interpolation mode, or the time range.
   * - ``ArraystructContext``
     - ``CTX_ARRAYSTRUCT_TYPE`` (103)
     - ``al_begin_arraystruct_action``. Holds the AoS path, its timebase,
       the current element index and a link to its parent AoS context, if
       any.

Contexts form a tree: a data entry has operations, an operation has
arrays of structures, and an array of structures can have nested arrays of
structures. Child contexts keep raw pointers to their parents.
``al_end_action`` removes a handle from the table and deletes the context;
when the context is a data entry context, it also deletes the backend.

A typical sequence for writing and reading back one IDS is:

.. code-block:: text

   al_begin_dataentry_action(uri, FORCE_CREATE_PULSE) -> pulse handle P
     al_begin_global_action(P, "equilibrium", "", WRITE_OP) -> operation O
       al_write_data(O, "ids_properties/homogeneous_time", "", ...)
       al_begin_arraystruct_action(O, "time_slice", "time", size=n) -> A
         al_write_data(A, "global_quantities/ip", "time_slice/time", ...)
         al_iterate_over_arraystruct(A, 1)        (repeated n-1 times)
       al_end_action(A)
     al_end_action(O)                             (backend may flush here)
     al_begin_global_action(P, "equilibrium", "", READ_OP) -> operation O2
       al_read_data(O2, "ids_properties/homogeneous_time", "", ...)
     al_end_action(O2)
   al_close_pulse(P, CLOSE_PULSE)
   al_end_action(P)                               (deletes the backend)

Backend selection
-----------------

The backend is chosen from the URI path (``imas:hdf5?...``) or, when the
URI has a host, set to UDA. The UDA backend can redirect to a local backend
for hosts listed in ``IMAS_LOCAL_HOSTS``, or download the files and then
delegate every call to a local backend (fetch mode). See :doc:`uri` and
:doc:`backends`.

Backends declare two capabilities: ``supportsTimeDataInterpolation`` (slice
reads) and ``supportsTimeRangeOperation`` (time range reads). For either,
the backend gets a shared interpolation component
(``src/data_interpolation.cpp``). Some backends implement interpolation
themselves (MDSplus, Memory). See :doc:`time`.

Versioning
----------

Two versions are relevant:

- **The IMAS-Core version** (``getALVersion()``, ``AL_VERSION``), from the
  git tag at build time.
- **The backend format version**, a ``major.minor`` pair per backend
  (``Backend::getVersion``). It is written into each data entry when the
  entry is created and compared with the compiled one when the entry is
  opened and before each write. See :ref:`REQ-DE-010` and
  :ref:`REQ-DE-011`.

IMAS-Core does not depend on a Data Dictionary version. The deprecated
``DD_VERSION`` macro is set to ``"!!DEPRECATED!!"``. High-level interfaces
store and check the Data Dictionary version themselves, as ordinary data
(``ids_properties/version_put/data_dictionary``).
