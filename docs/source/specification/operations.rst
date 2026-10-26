Operations
==========

An operation reads or writes one occurrence of one IDS in a data entry. It
starts with one of three functions and ends with ``al_end_action``:

- ``al_begin_global_action(pctx, ids, datapath, rwmode, &octx)`` for the
  whole IDS (all times);
- ``al_begin_slice_action(pctx, ids, rwmode, time, interp, &octx)`` for one
  time slice;
- ``al_begin_timerange_action(pctx, ids, rwmode, tmin, tmax, dtime,
  dtime_shape, interp, &octx)`` for the times between ``tmin`` and
  ``tmax``, optionally resampled.

This page specifies how operations are started and validated. What is read
or written inside an operation is in :doc:`data_access` and :doc:`aos`. The
time semantics of slices and time ranges are in :doc:`time`.

Common rules
------------

.. _REQ-OP-001:

REQ-OP-001: Start an operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Each ``al_begin_*_action`` function on a data entry handle
            shall check its arguments, let the backend start the
            operation, notify the plugins bound to the IDS and return status
            0 with a new operation handle.
:Nominal: ``al_begin_global_action(P, "equilibrium", "", READ_OP, &O)``
          returns an operation handle ``O``.
:Errors: An unknown handle fails with ``LOWLEVEL_ERR`` ``Cannot find
         context <n> in store``. A handle that is not a data entry handle
         fails with ``LOWLEVEL_ERR`` ``Wrong Context type stored``.
         Argument errors are listed below. On error, no handle is returned.
:Status: Implemented. Resource leaks on error are covered by
         :ref:`REQ-OP-021`.
:Trace: ``src/al_lowlevel.cpp:937-1108`` (``al_plugin_begin_*``),
        ``:1416-1550`` (``al_begin_*``). IMAS-Python
        ``imas/backends/imas_core/al_context.py:74, 100, 121``.

.. _REQ-OP-002:

REQ-OP-002: Access mode
^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``rwmode`` shall be ``READ_OP`` (30) or ``WRITE_OP`` (31).
:Errors: Any other value fails with ``CONTEXT_ERR`` ``Wrong access mode
         <n>``.
:Limits: ``REPLACE_OP`` (32) is defined, passes the check, and is passed to
         the backend. Only UDA translates it (to ``replace``); the other
         backends treat it as neither read nor write.
:Status: Unspecified for ``REPLACE_OP``: it should either be removed (and
         refused) or specified. It passes the check only because the list
         of valid access modes is declared with three slots and filled with
         two, so the third slot holds 0.
:Trace: ``include/al_const.h:102-108``; ``src/al_context.cpp:304-310,
        328-333, 366-372``; ``src/uda/uda_utilities.hpp:71``.

.. _REQ-OP-003:

REQ-OP-003: IDS name and occurrence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``ids`` argument shall be ``<ids_name>`` for occurrence 0
            or ``<ids_name>/<n>`` for occurrence ``n``. The HLI functions
            shall treat ``<ids_name>/0`` as ``<ids_name>``.
:Nominal: ``"equilibrium"`` and ``"equilibrium/0"`` address the same data;
          ``"equilibrium/2"`` addresses occurrence 2.
:Limits: IMAS-Core does not check the IDS name against the Data
         Dictionary, nor that ``n`` is a non-negative number: the text is
         passed to the backend, which uses it to name files or tree nodes.
         Reading a non-numeric occurrence such as ``core_profiles/invalid``
         returns no data. Only the exact suffix ``/0`` is removed:
         ``/00`` is not. The ``al_plugin_begin_*`` functions do not remove
         it.
:Status: Implemented. Whether names and occurrences should be validated
         is an open question. IMAS-Python passes ``<ids>`` for occurrence 0
         and expects ``occurrence="invalid"`` to read as an empty IDS.
:Trace: ``src/al_lowlevel.cpp:1423-1426, 1466-1469, 1512-1515``.
        IMAS-Python ``imas/backends/imas_core/db_entry_al.py:186-188``,
        ``imas/test/test_get_sample.py``.

.. _REQ-OP-004:

REQ-OP-004: Plugin notification
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: After the operation is started in the backend, the HLI
            functions shall call the matching ``begin_*_action`` of every
            plugin bound to the IDS, with the new operation handle.
:Limits: A plugin that does not implement the time range interface gets no
         call for a time range operation; a message is printed instead.
:Errors: A plugin error is reported as ``LOWLEVEL_ERR``; the operation
         handle was already created.
:Status: Implemented. See :doc:`plugins`.
:Trace: ``src/al_lowlevel.cpp:1430-1437``, ``:471-485``.

Global operations
-----------------

.. _REQ-OP-010:

REQ-OP-010: Global operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_global_action`` shall start an operation on the
            whole IDS occurrence. In read mode, all data of the IDS can be
            read; in write mode, the data written replaces the stored data
            at the same paths.
:Nominal: The usual ``get`` and ``put`` of an IDS.
:Status: Implemented. Whether stored data that is not written again during
         the operation is kept is not part of this requirement: IMAS-Python
         deletes every node before writing, so it does not depend on it
         (see :doc:`data_access`).
:Trace: ``src/al_lowlevel.cpp:937-989``.

.. _REQ-OP-011:

REQ-OP-011: Data path of a global read
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``datapath`` shall restrict a global read to the subtree it
            names. An empty string means the whole IDS.
:Nominal: IMAS-Python reads ``ids_properties`` alone from UDA before
          reading the IDS, with ``datapath="ids_properties"``.
:Limits: Only the UDA backend uses ``datapath`` (the Memory backend passes
         it on internally). The other backends read the whole IDS, which
         gives the same data for the paths actually read.
:Status: Implemented as a UDA optimization; other backends ignore it.
:Trace: ``src/uda/uda_backend.cpp:1017``. IMAS-Python
        ``imas/backends/imas_core/db_entry_al.py:190, 248``.

Slice operations
----------------

.. _REQ-OP-020:

REQ-OP-020: Slice operation arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_slice_action`` shall accept ``interp`` among
            ``UNDEFINED_INTERP`` (0), ``CLOSEST_INTERP`` (1),
            ``PREVIOUS_INTERP`` (2) and ``LINEAR_INTERP`` (3). A read
            shall require an interpolation other than ``UNDEFINED_INTERP``.
            A write ignores ``time`` and ``interp`` and appends the slice
            (see :doc:`time`).
:Nominal: IMAS-Python ``get_slice`` passes the requested time and mode;
          ``put_slice`` passes ``UNDEFINED_TIME`` and ``UNDEFINED_INTERP``.
:Errors: An unknown ``interp`` fails with ``CONTEXT_ERR`` ``Wrong interp mode
         <n>``. A read with ``UNDEFINED_INTERP`` fails with ``CONTEXT_ERR``
         ``Missing interpmode``. A backend that does not support slices
         fails with ``BACKEND_ERR``: ASCII (``ASCII Backend does not
         support slice mode of operation!``) and Flexbuffers
         (``Serialize Backend does not support slice mode``).
:Status: Implemented. Memory supports slices although its capability flag
         says it does not; see :doc:`backends`.
:Trace: ``src/al_context.cpp:316-351``; ``src/ascii_backend.cpp:192``;
        ``src/flexbuffers_backend.cpp:156``. Tests
        ``CapabilityMatrix.SliceReadPositiveOrRefused``,
        ``CapabilityMatrix.SliceWriteBeginPositiveOrRefused``,
        ``Hdf5TimeDependent.SliceInterpolationModes``,
        ``Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected`` (PR #95).
        IMAS-Python ``imas/backends/imas_core/db_entry_al.py:328-330``.

.. _REQ-OP-021:

REQ-OP-021: No leak when an operation cannot start
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When an ``al_begin_*_action`` function fails, it shall leave
            no operation context, no handle and no backend action open.
:Status: Defect, resource leak. When the backend refuses the operation
         (for example a slice on ASCII), the ``OperationContext`` object is
         leaked. When the write version check of :ref:`REQ-DE-011` fails,
         the backend action was already started and is never ended, so the
         backend can keep files open or state set.
:Trace: ``src/al_lowlevel.cpp:950-968``, ``:1004-1023``, ``:1067-1087``.

Time range operations
---------------------

.. _REQ-OP-030:

REQ-OP-030: Time range operation arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_timerange_action`` shall start a read of the times
            ``t`` with ``tmin <= t <= tmax``. ``dtime`` (``*dtime_shape``
            values) is empty for no resampling, or gives the resampling
            step or times. ``interp`` shall be one of ``CLOSEST_INTERP``,
            ``PREVIOUS_INTERP`` and ``LINEAR_INTERP`` when ``dtime`` is not
            empty, and may be ``UNDEFINED_INTERP`` otherwise.
:Nominal: HDF5 returns the stored samples between ``tmin`` and ``tmax``, or
          the data interpolated on the new time base.
:Errors: A backend that does not support time ranges fails with
         ``LOWLEVEL_ERR`` ``Selected backend does not support time range
         operations.`` (Memory, ASCII, Flexbuffers, MDSplus; UDA when the
         server plugin version is 1.4.0 or older). A non-empty ``dtime``
         with ``UNDEFINED_INTERP`` fails with ``CONTEXT_ERR`` ``Missing
         interpmode (dtime.size() != 0)``.
:Status: Implemented. Semantics of the result are in :doc:`time`.
:Trace: ``src/al_lowlevel.cpp:1045-1108``; ``src/al_context.cpp:353-388``.
        Tests ``CapabilityMatrix.TimeRangeReadPositiveOrRefused`` (PR #95),
        ``Hdf5TimeDependent.TimeRangeReadWithoutResampling``,
        ``Hdf5TimeDependent.TimeRangeReadWithResampling``. IMAS-Python
        ``get_sample`` (``imas/backends/imas_core/al_context.py:121``).

.. _REQ-OP-031:

REQ-OP-031: Time range argument checks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_timerange_action`` shall fail with ``CONTEXT_ERR``
            when ``interp`` is not one of the four interpolation constants,
            when ``tmin > tmax``, or when ``dtime_shape`` is ``NULL``.
:Status: Defect.

         - ``interp`` is not checked: any integer is stored.
         - ``tmin > tmax`` is not checked in C. The Python binding raises
           ``ValueError`` for it, and IMAS-Python relies on that.
         - A ``NULL`` ``dtime_shape`` is dereferenced (crash).
:Trace: ``src/al_context.cpp:353-388``; ``src/al_lowlevel.cpp:1063,
        1509``; ``python/imas_core/_al_lowlevel.pyx:456-462``. IMAS-Python
        ``imas/test/test_get_sample.py:108-114``.

.. _REQ-OP-032:

REQ-OP-032: Complete time range context
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A time range operation context shall have a defined time
            (``UNDEFINED_TIME``) and interpolation mode (the ``interp``
            argument) so that backends reading them get defined values.
:Status: Defect, wrong result. The time range constructor sets neither,
         so they hold indeterminate values. The UDA backend sends them to
         the server, and time range reads through UDA fail with ``unknown
         interp mode``.
:Trace: ``src/al_context.cpp:353-388``; ``include/al_context.h:373-376``;
        ``src/uda/uda_backend.cpp:680-681``. UDA tier xfail (PR #97).
