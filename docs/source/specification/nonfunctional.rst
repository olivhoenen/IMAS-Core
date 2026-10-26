Non-functional requirements
===========================

This page covers requirements that do not belong to one function:
concurrency, memory ownership, configuration through the environment,
console output, and the stability promises that high-level interfaces rely
on.

Concurrency
-----------

.. _REQ-NFR-001:

REQ-NFR-001: Use from several threads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall document which calls may run at the same time
            from several threads. Until it does, a program shall not call
            IMAS-Core from more than one thread at a time.
:Nominal: A single-threaded program, or one that serializes its calls
          with its own lock, is supported.
:Limits: The baseline gives only partial protection:

         - Creating and freeing context handles is locked, but looking up a
           handle is not. A lookup can therefore race with a table resize
           in another thread.
         - The plugin registry and bindings are global and have no lock.
         - The MDSplus backend changes the process environment variable
           ``ids_path`` each time it opens a tree, then restores it. Two
           threads opening MDSplus data entries interfere. So does any
           other code in the process that relies on ``ids_path``.
         - The Memory backend locks its global store when opening, but see
           REQ-NFR-002.
:Status: Unspecified. No thread-safety promise is documented, and the
         baseline is not thread-safe.
:Trace: ``src/al_lowlevel.cpp:537-587`` (``addLLenv``, ``getLLenv``,
        ``delLLenv``), ``src/al_lowlevel.cpp:46-53`` (plugin registries),
        ``src/mdsplus/mdsplus_backend.cpp:1486-1517``.

.. _REQ-NFR-002:

REQ-NFR-002: Memory backend locking
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The lock of a Memory store shall be released by its ``unlock``
            operation.
:Status: Defect. ``InternalCtx::unlock`` calls ``pthread_mutex_lock``
         instead of ``pthread_mutex_unlock``. The mutex is recursive, so a
         single thread does not notice. A second thread that uses the same
         store blocks forever.
:Trace: ``src/memory_backend.h:354-361``.

Memory ownership
----------------

.. _REQ-NFR-010:

REQ-NFR-010: Memory returned to the caller
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Memory that IMAS-Core returns through an output pointer shall be
            allocated with ``malloc``. The caller owns it and shall release
            it with ``free``. This applies to:

            - the data of an array or string read by ``al_read_data``;
            - the string from ``al_context_info``;
            - the URI from ``al_build_uri_from_legacy_parameters``;
            - the list from ``al_get_occurrences``;
            - the list from ``al_list_filled_paths``, and each string in it.

            Scalars are written into the caller's own variable.
:Nominal: The Python binding hands read arrays to numpy, which frees them,
          and frees all other results itself.
:Limits: When nothing is returned (no data, no occurrences), the pointer is
         ``NULL`` and the size is 0. ``free(NULL)`` is safe.
:Status: Implemented.
:Trace: ``include/al_lowlevel.h:243, 468, 487, 491, 505``. Contract tests
        ``UriOwnership.CallerFreesBuiltUri``, ``ContextInfo.*``,
        ``CapabilityMatrix.ListFilledPathsPositiveOrRefused``,
        ``Occurrences.*``. ``python/imas_core/_al_lowlevel.pyx:912-915,
        1219-1222, 1242-1246``.

Handle lifetime, leaks on errors and stale handles are specified in
:doc:`contexts`.

Environment and output
----------------------

.. _REQ-NFR-020:

REQ-NFR-020: Environment variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Every environment variable that changes IMAS-Core behavior
            shall be listed in :doc:`../user_guide/configuration`. When a
            URI option and an environment variable set the same thing, the
            URI option shall win.
:Nominal: The baseline reads the following variables:

          .. list-table::
             :header-rows: 1
             :widths: 35 45 20

             * - Variable
               - Effect
               - Documented
             * - ``IMAS_HOME``
               - Root of ``public`` legacy data entries (:doc:`uri`)
               - No
             * - ``IMAS_LOCAL_HOSTS``
               - UDA hosts redirected to local access (:doc:`uri`)
               - Yes
             * - ``IMAS_AL_ENABLE_PLUGINS``
               - Enables the plugin framework (REQ-PLG-001)
               - Yes
             * - ``IMAS_AL_PLUGINS``
               - Directory of plugin libraries (REQ-PLG-002)
               - No
             * - ``IMAS_AL_DISABLE_INTERPOLATION_WARNING``
               - Silences the HDF5 and MDSplus interpolation warning
                 (REQ-BE-014)
               - No
             * - ``HDF5_BACKEND_READ_CACHE``, ``HDF5_BACKEND_WRITE_CACHE``
               - HDF5 cache sizes in MiB, overridden by the URI options
               - Yes
             * - ``MDSPLUS_MODELS_PATH``
               - MDSplus model trees (REQ-BE-030)
               - Yes
             * - ``IMAS_UDA_PLUGIN``
               - Default UDA server plugin (REQ-BE-050)
               - No
             * - ``IDSDEF_PATH``, ``IMAS_PREFIX``
               - Data Dictionary used by the UDA backend (REQ-BE-051)
               - No
             * - ``UDA_HOST``, ``UDA_PORT`` and the UDA client variables
               - UDA server when the URI has no host
               - Yes
             * - ``USER``, ``TMPDIR``
               - Default UDA fetch cache directory (REQ-BE-053)
               - No
:Limits: ``IMAS_AL_DISABLE_OBSOLESCENT_WARNING`` and
         ``IMAS_AL_SERIALIZER_TMP_DIR`` are documented on the same page but
         are read by the high-level interfaces, not by IMAS-Core.
:Status: Defect (documentation). Six of the variables above are not
         documented.
:Trace: ``grep getenv src/``. :doc:`../user_guide/configuration`.

.. _REQ-NFR-021:

REQ-NFR-021: Changes to the process environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core should not change the process environment. Where a
            backend library needs it (MDSplus finds trees through
            ``<tree>_path``), IMAS-Core shall restore the previous value
            when it is done.
:Nominal: The MDSplus backend sets ``ids_path`` to
          ``<path>;$MDSPLUS_MODELS_PATH`` while opening or creating a tree,
          then resets it.
:Limits: The tree name can be changed with the ``DEF_TREENAME`` query key,
         but the variable set is always ``ids_path``.
:Status: Implemented. Concurrency limits are in REQ-NFR-001.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:1420-1517``.

.. _REQ-NFR-022:

REQ-NFR-022: Console output
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core should report errors only through the status (or the
            Python exception), and print only warnings that the user can
            turn off.
:Status: Defect (cosmetic). The baseline also prints to standard output or
         standard error in these cases:

         - A ``LOWLEVEL_ERR`` in ``al_read_data`` prints
           "-->status.message = ...".
         - Binding a plugin to all nodes of an IDS prints a line.
         - ``al_unbind_plugin`` with an unknown path prints "No plugin bound
           to field path: ...".
         - A failed ``dlopen`` prints "An error has occurred: ...".
         - An unknown backend identifier prints "Non-identified backend ID".
         - The UDA backend prints a trace when ``verbose=1``. This one is
           intended.
:Trace: ``src/al_lowlevel.cpp:220, 304, 354, 1231``,
        ``src/al_backend.cpp:84``.

Compatibility and stability
---------------------------

.. _REQ-NFR-030:

REQ-NFR-030: API versioning
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall follow semantic versioning:

            - A minor release may add functions, constants and URI options.
            - Removing or changing a function, constant, return shape,
              status code or error class needs a new major version.
            - The shared library's ``SOVERSION`` is the major version.

            The features below are detected by IMAS-Python, by checking
            the version or whether a function exists. They shall keep their
            names and signatures within major version 5.
:Nominal: The following table lists these features:

          .. list-table::
             :header-rows: 1
             :widths: 45 15 40

             * - Feature
               - Since
               - How IMAS-Python detects it
             * - ``al_begin_dataentry_action``,
                 ``al_build_uri_from_legacy_parameters``, plugin functions
               - 5.0
               - The ``al_<name>`` function exists
             * - ``get_al_version``
               - after 5.0
               - The function exists, otherwise 5.0.0 is assumed
             * - ``al_get_occurrences``
               - 5.1
               - The function exists; the ``imas convert`` command requires
                 version 5.1
             * - Errors raised as exceptions (breaking change)
               - 5.2
               - ``imas_core_config_enable_exceptions`` exists
             * - ``FLEXBUFFERS_SERIALIZER_PROTOCOL``
               - 5.3
               - The constant exists
             * - ``al_begin_timerange_action``
               - 5.4
               - The function exists
             * - UDA ``cache_mode=none``
               - 5.5.1
               - Version number (shown in a hint only)
             * - MDSplus models found through ``MDSPLUS_MODELS_PATH``
               - 5.6.0
               - Version number
             * - ``al_list_filled_paths``
               - 5.7
               - The function exists
:Status: Implemented. Some changes listed elsewhere in this document, such
         as making return shapes uniform (REQ-PY-020) or changing error
         codes (see :doc:`errors`), are breaking changes under this rule.
:Trace: ``CMakeLists.txt:201`` (``SOVERSION``). IMAS-Python
        ``imas/backends/imas_core/imas_interface.py:23-118``,
        ``imas/ids_defs.py:128-130``, ``imas/backends/imas_core/mdsplus_model.py:298-309``,
        ``imas/backends/imas_core/db_entry_al.py:384-398``,
        ``docs/source/changelog.rst:258-267`` (5.2 exceptions).

.. _REQ-NFR-031:

REQ-NFR-031: Stored data compatibility
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Data written by any 5.x release shall stay readable by every
            later 5.x release with the same backend. A change to a backend
            format that older releases cannot read shall increase that
            backend's format version (REQ-BE-003).
:Nominal: HDF5 data entries carry format 1.0 throughout 5.x.
:Limits: IMAS-Core does not convert data between Data Dictionary
         versions. It stores ``ids_properties/version_put/data_dictionary``
         as plain data and never reads it. Conversion is the job of the
         high-level interface.
:Status: Implemented.
:Trace: :doc:`data_entry`. Contract test
        ``Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted``.
        IMAS-Python converts between Data Dictionary versions itself
        (``imas/backends/imas_core/db_entry_al.py:288-296``).
