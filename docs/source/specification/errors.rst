Errors, status codes and messages
=================================

This page specifies how IMAS-Core reports errors. The status codes are
introduced in :doc:`../user_guide/error_handling`. The Python exceptions
are covered in more detail in :doc:`python_binding`.

Status and codes
----------------

.. _REQ-ERR-001:

REQ-ERR-001: Status structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Every function of the C API except ``const2str``, ``err2str``,
            ``getALVersion`` and ``getDDVersion`` shall return an
            ``al_status_t`` holding an integer ``code`` and a
            NUL-terminated ``message`` of at most ``MAX_ERR_MSG_LEN`` (256)
            bytes. ``code`` is 0 on success and negative on error.
:Nominal: ``if (status.code < 0) fprintf(stderr, "%s\n", status.message);``
:Limits: A message longer than 255 characters is truncated.
:Status: Implemented.
:Trace: ``include/al_lowlevel.h:230-234``; ``include/al_defs.h.in``.

.. _REQ-ERR-002:

REQ-ERR-002: Meaning of the codes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The status code shall tell where the error was found:

            .. list-table::
               :header-rows: 1
               :widths: 15 20 65

               * - Code
                 - Constant
                 - Used for
               * - ``-2``
                 - ``CONTEXT_ERR``
                 - Invalid arguments of an operation or URI: unknown
                   access, range or interpolation mode, missing
                   interpolation, unparsable URI, unknown backend name,
                   missing legacy key.
               * - ``-3``
                 - ``BACKEND_ERR``
                 - Errors raised by the storage backend: missing or
                   existing data entry, file access, unsupported feature
                   of that backend, backend not built.
               * - ``-4``
                 - ``LOWLEVEL_ERR``
                 - Errors of the core: unknown handle, wrong handle type,
                   version incompatibility, unsupported time range, rank
                   mismatch on read, plugin errors.
               * - ``-1``
                 - ``UNKNOWN_ERR``
                 - Any other error, including exceptions not raised by
                   IMAS-Core itself (standard library, UDA client).
:Status: Implemented, with the exceptions of :ref:`REQ-ERR-003`,
         :ref:`REQ-ERR-004` and :ref:`REQ-ERR-005`.
:Trace: ``include/al_defs.h.in``; ``include/al_const.h`` (``alerror``).

.. _REQ-ERR-003:

REQ-ERR-003: Context errors keep their code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An ``ALContextException`` shall be reported as
            ``CONTEXT_ERR`` by every function.
:Nominal: ``al_begin_slice_action`` with an unknown interpolation mode
          returns ``CONTEXT_ERR``.
:Status: Defect, wrong result. ``al_begin_dataentry_action`` has no handler
         for context exceptions, so URI errors (unparsable URI, unknown
         backend name, both ``path`` and ``mapping``, missing legacy key)
         are returned as ``UNKNOWN_ERR``. The contract tests pin this
         current code. ``al_close_pulse``, ``al_delete_data``,
         ``al_get_occurrences``, ``al_list_filled_paths``,
         ``al_plugin_read_data``, ``al_plugin_write_data`` and
         ``al_plugin_end_action`` have no handler either, but do not raise
         context exceptions today.
:Trace: ``src/al_lowlevel.cpp:885-897``. Tests
        ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``
        (``UNKNOWN_ERR``, PR #98).

.. _REQ-ERR-004:

REQ-ERR-004: Backend errors keep their code
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An ``ALBackendException`` shall be reported as ``BACKEND_ERR``
            by every function, including ``al_end_action``.
:Nominal: An MDSplus write that fails when the operation is flushed at
          ``al_end_action`` returns ``BACKEND_ERR``.
:Status: Defect, wrong result. The HLI ``al_end_action`` has no handler
         for backend exceptions, so they are returned as ``UNKNOWN_ERR``
         (``al_plugin_end_action`` returns ``BACKEND_ERR``). Backend
         exceptions raised inside plugin code called from the HLI
         ``al_begin_*``, ``al_read_data`` and ``al_write_data`` are also
         returned as ``UNKNOWN_ERR``; backend errors from the core path keep
         ``BACKEND_ERR`` because the status of the ``al_plugin_*`` call is
         returned as is.
:Trace: ``src/al_lowlevel.cpp:1640-1675`` versus ``:1110-1141``.

.. _REQ-ERR-005:

REQ-ERR-005: Errors of al_build_uri_from_legacy_parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_build_uri_from_legacy_parameters`` reports an unsupported
            backend id as ``LOWLEVEL_ERR``.
:Status: Implemented, but inconsistent with :ref:`REQ-ERR-003`: the
         exception is a context exception mapped to ``LOWLEVEL_ERR``. The
         contract tests pin ``LOWLEVEL_ERR``; changing it to
         ``CONTEXT_ERR`` would be a visible change.
:Trace: ``src/al_lowlevel.cpp:1370-1374, 1408-1412``. Test
        ``BackendFactory.UnknownPositiveLegacyBackendIdIsRefused`` (PR #98).

.. _REQ-ERR-006:

REQ-ERR-006: Plugin errors
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An ``ALPluginException`` shall be reported as ``LOWLEVEL_ERR``;
            there is no separate plugin code.
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp`` (``catch (const ALPluginException&)`` in
        the HLI functions). See :doc:`plugins`.

.. _REQ-ERR-007:

REQ-ERR-007: No exception crosses the C API
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: No C++ exception shall propagate out of a function of the C
            API; every exception shall be turned into a status.
:Status: Defect, crash (latent). ``al_context_info`` and
         ``al_get_backendID`` only catch ``ALLowlevelException``, and
         ``al_build_uri_from_legacy_parameters`` only catches
         ``ALContextException``. Any other exception (for example
         ``std::bad_alloc``) leaves through an ``extern "C"`` function and
         terminates the process.
:Trace: ``src/al_lowlevel.cpp:801-854``, ``:1349-1414``.

Messages
--------

.. _REQ-ERR-010:

REQ-ERR-010: Message content
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The message of a failed call shall be
            ``<function>: [<ExceptionClass> = <description>]``, where
            ``<function>`` is the C function that caught the error and
            ``<ExceptionClass>`` is ``ALException``,
            ``ALLowlevelException``, ``ALBackendException``,
            ``ALContextException`` or ``ALPluginException``. Builds with
            ``DEBUG`` defined shall add the source file and line:
            ``[<ExceptionClass> (<file>:<line>) = <description>]``.
:Nominal: ``al_plugin_begin_global_action: [ALLowlevelException = Cannot
          find context 42 in store]``. When an HLI function returns the
          status of the ``al_plugin_*`` function it called, the message
          names that inner function.
:Limits: The description of exceptions from other libraries (MDSplus, HDF5,
         UDA, standard library) is their ``what()`` text, without brackets.
         Long descriptions are truncated (:ref:`REQ-ERR-001`). Messages are
         meant for people; callers shall not parse them, except for the
         wording that IMAS-Python relies on (:ref:`REQ-URI-011`).
:Status: Implemented.
:Trace: ``src/al_exception.cpp:7-56`` (``registerStatus``);
        ``include/al_exception.h:6-12``. Python test
        ``python/tests/test_exception.py``.

.. _REQ-ERR-011:

REQ-ERR-011: Message on success
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: On success, ``message`` shall be an empty string, unless the
            call succeeded with a warning (:ref:`REQ-ERR-012`).
:Status: Defect, wrong result. Only ``al_begin_dataentry_action``
         initializes the status. The other functions leave ``message``
         uninitialized on success, so a caller that prints it gets
         arbitrary bytes, and a caller cannot tell a warning from garbage.
:Trace: ``src/al_lowlevel.cpp:858`` (``al_status_t status = { 0 };``)
        versus, for example, ``:910-912``.

.. _REQ-ERR-012:

REQ-ERR-012: Warnings
^^^^^^^^^^^^^^^^^^^^^

:Statement: A call that succeeds but changed the request shall return code
            0 and a message starting with
            ``<function>: [ALLowlevelException = Warning:``.
:Nominal: Reading an integer field as double converts the data and returns
          code 0 with a warning that names the URI, the field and both
          types (see :doc:`data_access`).
:Status: Implemented for type conversion on read, which is the only
         warning.
:Trace: ``src/al_lowlevel.cpp:1209-1218``.

.. _REQ-ERR-013:

REQ-ERR-013: No output on the standard streams
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall report errors through the status only, and
            shall not write to standard output or standard error unless a
            verbose or debug option is set.
:Status: Defect, cosmetic. Among others, ``al_plugin_read_data`` prints
         ``-->status.message = ...`` on every ``LOWLEVEL_ERR``,
         ``Backend::initBackend`` prints an unknown backend id to standard
         error, ``LLplugin::beginTimeRangeActionPlugin`` prints a message
         for plugins without the time range interface, and backends print
         warnings (for example the HDF5 interpolation warning, which can be
         switched off with ``IMAS_AL_DISABLE_INTERPOLATION_WARNING``).
:Trace: ``src/al_lowlevel.cpp:1231``, ``:481``; ``src/al_backend.cpp:83``.

Exceptions
----------

.. _REQ-ERR-020:

REQ-ERR-020: C++ exception classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The C++ exception classes shall form this hierarchy, and each
            shall map to one status code:

            .. code-block:: text

               std::runtime_error
                 ALException                -> UNKNOWN_ERR
                   ALContextException       -> CONTEXT_ERR
                   ALBackendException       -> BACKEND_ERR
                   ALLowlevelException      -> LOWLEVEL_ERR
                   ALPluginException        -> LOWLEVEL_ERR

            ``what()`` shall return the bracketed text of
            :ref:`REQ-ERR-010`.
:Status: Implemented, apart from :ref:`REQ-ERR-003` and
         :ref:`REQ-ERR-004`.
:Trace: ``include/al_exception.h:25-99``.

.. _REQ-ERR-021:

REQ-ERR-021: Python exceptions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The Python binding shall raise, for a non-zero code, an
            exception of class ``ImasCoreContextException`` (``-2``),
            ``ImasCoreBackendException`` (``-3``), ``ImasCoreException``
            (``-4``) or ``ALException`` (``-1`` and any other code). All
            four classes derive from ``imas_core.exception.ALException``
            and carry ``message`` and ``errorStatus``.
:Nominal: Opening ``imas:mdsplus?path=x`` in a build without MDSplus raises
          ``ImasCoreBackendException`` whose message contains
          ``not available``.
:Limits: Whether a function raises or returns its status depends on the
         function and on ``imas_core_config_enable_exceptions()``; see
         :doc:`python_binding`.
:Status: Implemented.
:Trace: ``python/imas_core/exception.py``;
        ``python/imas_core/_al_lowlevel.pyx:158-185``. Test
        ``python/tests/test_exception.py``. IMAS-Python
        ``imas/exception.py:24``, ``imas/test/conftest.py:61-67``.
