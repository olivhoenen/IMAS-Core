Python binding
==============

This page specifies the ``imas_core`` Python package, a Cython wrapper of the
C API. The main user is IMAS-Python, which calls it through a thin
compatibility layer (``imas/backends/imas_core/imas_interface.py``). Where a
behavior matters to IMAS-Python, the trace names the IMAS-Python file. All
IMAS-Python assumptions are collected in :doc:`consumers`.

Behaviors marked *verified* were checked by running the installed
``imas_core`` 5.7.2 on the Memory backend.

Package and constants
---------------------

.. _REQ-PY-001:

REQ-PY-001: Package layout
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The package ``imas_core`` shall provide:

            - ``_al_lowlevel``, the wrappers of the C functions;
            - ``imasdef``, all constants;
            - ``exception``, the exception classes;
            - ``__version__`` and ``version_tuple``.

            ``_al_lowlevel.get_al_version()`` shall return the library
            version as ``MAJOR.MINOR.PATCH``, equal to the first three
            fields of ``version_tuple``.
:Nominal: ``imas_core.imasdef.AL_VERSION.decode()`` equals
          ``"5.7.2"`` for release 5.7.2.
:Limits: ``imasdef.AL_VERSION`` and ``imasdef.DD_VERSION`` are ``bytes``.
         ``DD_VERSION`` is always ``b"!!DEPRECATED!!"``.
:Status: Implemented.
:Trace: ``python/imas_core/__init__.py``, ``python/imas_core/al_defs.pyx:60-61``,
        ``python/imas_core/_al_lowlevel.pyx:1227-1229``.
        ``python/tests/test_import.py``, ``python/tests/test_imasdef.py``.
        IMAS-Python parses the version with ``packaging.Version`` and
        assumes 5.0.0 when ``get_al_version`` is missing
        (``imas/backends/imas_core/imas_interface.py:68-74``).

.. _REQ-PY-002:

REQ-PY-002: Constants
^^^^^^^^^^^^^^^^^^^^^

:Statement: ``imasdef`` shall export every constant of ``al_defs.h``, with
            the same values, plus the Python-level constants: ``EMPTY_INT``
            (``-999999999``), ``EMPTY_FLOAT`` and ``EMPTY_DOUBLE``
            (``-9.0e40``), ``EMPTY_COMPLEX``, ``IDS_TIME_MODE_UNKNOWN``
            (``-999999999``), ``IDS_TIME_MODE_HETEROGENEOUS`` (0),
            ``IDS_TIME_MODE_HOMOGENEOUS`` (1), ``IDS_TIME_MODE_INDEPENDENT``
            (2), ``IDS_TIME_MODES`` and the ``NODE_TYPE_*`` values.
:Limits: ``TIMERANGE_OP`` and ``FLEXBUFFERS_BACKEND`` are not exported.
         ``UNDEFINED_TIME`` is declared as a C ``float``.
:Status: Defect (minor). The two missing constants should be exported.
:Trace: ``python/imas_core/al_defs.pxd``, ``python/imas_core/al_defs.pyx``,
        ``python/imas_core/imasdef.py:63-113``. IMAS-Python reads the
        serializer protocols with ``getattr`` fallbacks
        (``imas/ids_defs.py:96-131``).

Errors
------

.. _REQ-PY-010:

REQ-PY-010: Exception classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A failed call shall raise an exception whose class depends on
            the status code:

            - ``CONTEXT_ERR`` raises ``ImasCoreContextException``.
            - ``BACKEND_ERR`` raises ``ImasCoreBackendException``.
            - ``LOWLEVEL_ERR`` raises ``ImasCoreException``.
            - ``UNKNOWN_ERR``, and any other code, raises ``ALException``.

            All four classes derive from ``ALException``, which stores
            ``message`` and ``errorStatus``.
:Nominal: ``str(exc)`` is ``"<message>\nError status=<code>"``.
:Limits: When ``errorStatus`` is 0 or ``None``, ``str(exc)`` is only the
         message.
:Status: Implemented.
:Trace: ``python/imas_core/exception.py``,
        ``python/imas_core/_al_lowlevel.pyx:167-185``.
        ``python/tests/test_exception.py``. IMAS-Python exposes
        ``imas.exception.ALException`` as ``lowlevel.ALException``
        (``imas/exception.py:24``) and does not catch the subclasses.

.. _REQ-PY-011:

REQ-PY-011: Exception mode
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``imas_core_config_enable_exceptions()`` and
            ``imas_core_config_disable_exceptions()`` shall switch, for the
            whole process, between raising an exception on error and
            returning the status code. In both modes, the result of a
            successful call shall be the same.
:Nominal: IMAS-Python enables exceptions when it is imported
          (``imas/backends/imas_core/imas_interface.py:23-25``).
:Limits: Exceptions are disabled by default. In disabled mode, the error
         message is also logged with ``logging.error``.
:Status: Defect. The switch only affects ``al_context_info``,
         ``al_get_backendID``, ``al_build_uri_from_legacy_parameters``,
         ``al_begin_dataentry_action``, ``al_close_pulse``,
         ``al_begin_global_action``, ``al_begin_slice_action``,
         ``al_end_action`` and ``al_begin_arraystruct_action``. All other
         functions always raise. ``al_begin_timerange_action`` always raises
         a plain ``ALException``, whatever the status code.
:Trace: ``python/imas_core/_al_lowlevel.pyx:158-164`` and each function
        listed.

.. _REQ-PY-012:

REQ-PY-012: Error messages are text
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``message`` of an exception shall be a ``str`` holding the C
            status message.
:Status: Defect (cosmetic). The functions that obey the exception mode
         (REQ-PY-011) and ``al_begin_timerange_action`` pass the raw
         ``bytes``. Their ``message`` is ``bytes`` and is shown as
         ``b'...'`` (*verified*). The other functions decode the message and
         add a prefix such as "Error while reading data: ". ``al_delete_data``
         and ``al_iterate_over_arraystruct`` also use that read prefix.
:Trace: ``python/imas_core/_al_lowlevel.pyx:206, 318, 486, 1022, 1088``.

Data entries and actions
------------------------

.. _REQ-PY-020:

REQ-PY-020: Return values of the context functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Each wrapper shall return values of the same shape whether the
            call succeeds or fails. The baseline shapes are:

            .. list-table::
               :header-rows: 1

               * - Function
                 - Success
                 - Failure with exceptions disabled
               * - ``al_begin_dataentry_action(uri, mode)``
                 - ``(0, ctx)``
                 - ``(code, ctx)``, ``ctx`` possibly allocated
               * - ``al_close_pulse(ctx, mode)``
                 - ``0``
                 - ``code``
               * - ``al_begin_global_action(ctx, ids, rw, datapath="")``
                 - ``(0, opctx)``
                 - ``(code, -1)``
               * - ``al_begin_slice_action(ctx, ids, rw, time, interp)``
                 - ``(0, opctx)``
                 - ``(code, -1)``
               * - ``al_begin_timerange_action(...)``
                 - ``opctx``
                 - always raises
               * - ``al_begin_arraystruct_action(ctx, path, timebase, size)``
                 - ``(0, aosctx, size)``
                 - ``(code, -1, 0)``
               * - ``al_end_action(ctx)``
                 - ``0``
                 - ``(code, -1)``
               * - ``al_context_info(ctx)``
                 - ``str``
                 - ``(code, "")``
               * - ``al_get_backendID(ctx)``
                 - ``int``
                 - ``(code, "")``
               * - ``al_build_uri_from_legacy_parameters(...)``
                 - ``(0, uri)``
                 - ``(code, -1)``
:Status: Defect. ``al_begin_timerange_action``, ``al_end_action``,
         ``al_context_info`` and ``al_get_backendID`` change shape on error.
         ``al_begin_timerange_action`` returns a bare context, while all the
         other ``begin`` functions return a tuple. Changing this would break
         IMAS-Python, which unpacks every ``begin`` call except the time
         range one (``imas/backends/imas_core/al_context.py:74-140``).
:Trace: ``python/imas_core/_al_lowlevel.pyx:197-518, 1048-1065``.

.. _REQ-PY-021:

REQ-PY-021: Time range arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_timerange_action(ctx, ids, rwmode, tmin, tmax, dtime,
            interpmode=None)`` shall check its arguments before calling the C
            function:

            - ``tmin > tmax`` raises ``ValueError``.
            - A ``dtime`` with no ``interpmode`` raises ``ValueError``.
            - A ``dtime`` that is neither ``None`` nor a numpy array raises
              ``ValueError``.

            ``dtime`` is converted to ``float64``. ``interpmode=None`` is
            sent as ``UNDEFINED_INTERP`` (0).
:Nominal: ``dtime=None`` reads the stored time points between ``tmin`` and
          ``tmax`` (see :doc:`time`).
:Limits: ``tmin == tmax`` is accepted. These checks exist only in Python;
         the C function does not check ``tmin > tmax``.
:Status: Implemented.
:Trace: ``python/imas_core/_al_lowlevel.pyx:440-488``. IMAS-Python's
        ``get_sample`` relies on these ``ValueError`` checks
        (``imas/test/test_get_sample.py:108-114``).

Writing data
------------

.. _REQ-PY-030:

REQ-PY-030: Python types accepted by ``al_write_data``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_write_data(ctx, path, timebase, value)`` shall choose the
            IMAS data type from the Python value and write it without
            losing information:

            .. list-table::
               :header-rows: 1

               * - Python value
                 - IMAS type and rank
               * - ``int``, ``bool``, numpy integer scalar
                 - ``INTEGER_DATA``, rank 0
               * - ``float``, ``numpy.float64``
                 - ``DOUBLE_DATA``, rank 0
               * - ``complex``
                 - ``COMPLEX_DATA``, rank 0
               * - ``str``
                 - ``CHAR_DATA``, rank 1 (UTF-8 bytes)
               * - ``list`` of ``str``
                 - ``CHAR_DATA``, rank 2
               * - numpy array of signed integers, floats or complex numbers
                 - ``INTEGER_DATA``, ``DOUBLE_DATA`` or ``COMPLEX_DATA``,
                   rank ``ndim``
               * - ``None``
                 - nothing written, unless a plugin is bound
:Nominal: The array is sent in Fortran order. A C-ordered array is copied.
:Limits: The following limits apply:

         - An array with a dimension of length 0 is not sent to the C
           layer, and the call returns 0.
         - ``None`` is sent with data type -1. The core then drops it
           unless a plugin is bound (see :doc:`data_access`).
:Errors: The following values are refused:

         - A numpy array of unsigned integers or booleans raises
           ``ALException`` "UNKNOWN DATA TYPE".
         - A ``numpy.float32`` scalar raises ``ALException`` "UNKNOWN DATA
           TYPE" (*verified*).
         - A list that is not all strings, or any other type, raises
           ``ALException`` "UNKNOWN DATA TYPE".
         - An ``int`` scalar outside the 32-bit range raises
           ``OverflowError`` (*verified*).
:Status: Defect (data loss):

         - Integer arrays are converted to ``int32`` without a range check,
           so ``int64`` values outside the 32-bit range are silently
           truncated (*verified*: ``2**40 + 5`` is stored as ``5``).
         - Strings in a list are measured in characters, not UTF-8 bytes, so
           non-ASCII strings are cut (*verified*: ``"é"`` reads back as
           ``"�"``).
         - A ``bytes`` item in a list fails, although the type check
           accepts it.
:Trace: ``python/imas_core/_al_lowlevel.pyx:44-57, 114-151, 553-790``.
        IMAS-Python's ``test_exception.py`` expects
        ``write_data(-1, "X", "", object())`` to raise ``ALException``.

.. _REQ-PY-031:

REQ-PY-031: Writing one slice
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_write_slice_data(ctx, path, timebase, value)`` shall write
            the first time slice of ``value``: index 0 of the last
            dimension of an array of rank 1 to 6, or the first string of a
            string list.
:Status: Defect. The function passes already encoded ``bytes`` to
         ``al_write_data``, then reads ``.code`` from the integer it
         returns. Every call raises ``AttributeError`` (*verified*).
         Arrays of rank 7 are refused. IMAS-Python does not use this
         function.
:Trace: ``python/imas_core/_al_lowlevel.pyx:662-690``.

Reading data
------------

.. _REQ-PY-040:

REQ-PY-040: Values returned by ``al_read_data``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_read_data(ctx, path, timebase, type, rank)`` shall return
            ``(status, value)``, where ``value`` is:

            .. list-table::
               :header-rows: 1

               * - Request
                 - Stored data
                 - No data
               * - numeric, rank 0
                 - Python scalar
                 - ``EMPTY_INT``, ``EMPTY_DOUBLE`` or ``EMPTY_COMPLEX``
               * - numeric, rank 1 to 7
                 - numpy array in Fortran order, owning the C buffer
                 - ``None``
               * - CHAR, rank 1
                 - ``str``
                 - ``""``
               * - CHAR, rank 2
                 - ``list`` of ``str``
                 - ``None``
:Nominal: An integer array is returned as ``int32`` (*verified*).
:Limits: The following limits apply:

         - A numeric array with any dimension of length 0 is returned as
           ``None``.
         - Strings are decoded as UTF-8, with invalid bytes replaced by
           U+FFFD.
         - Trailing NUL characters are removed from the strings of a list.
         - A CHAR rank 0 read returns the character as an ``int``.
:Status: Defect. CHAR reads of rank 3 or more use a two-element size buffer
         and overflow it. The backends that can store CHAR rank 3 data,
         such as Memory, can therefore corrupt memory.
:Trace: ``python/imas_core/_al_lowlevel.pyx:823-1000``. IMAS-Python relies
        on ``None`` for missing arrays and on the ``EMPTY_*`` values or
        ``""`` for missing scalars
        (``imas/backends/imas_core/db_entry_helpers.py:70-77, 119-130``).
        It reads strings with rank ``ndim + 1``
        (``imas/ids_metadata.py:257-258``).

.. _REQ-PY-041:

REQ-PY-041: Query results
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_get_occurrences(ctx, ids)`` shall return ``(0, array)``
            with the sorted occurrence numbers, or ``(0, None)`` when there
            are none. ``al_list_filled_paths(ctx, ids)`` shall return
            ``(0, list of str)``. The wrappers free the C memory.
:Errors: A backend without the query raises ``ImasCoreBackendException``
         (*verified* on Memory).
:Status: Implemented.
:Trace: ``python/imas_core/_al_lowlevel.pyx:1207-1248``. IMAS-Python turns
        ``None`` into an empty list
        (``imas/backends/imas_core/al_context.py:170-175``).

.. _REQ-PY-042:

REQ-PY-042: Flexbuffers buffer access
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The serialized buffer of the Flexbuffers backend shall be
            readable with ``al_read_data_array(ctx, "<buffer>", "",
            CHAR_DATA, 1)``, and writable with
            ``_al_write_data_array(ctx, b"<buffer>", b"", int8_array,
            CHAR_DATA)``, on the data entry context (REQ-BE-090).
:Status: Implemented. IMAS-Python calls the private
         ``_al_write_data_array`` directly
         (``imas/ids_toplevel.py:166, 203``). A public function for this
         should be provided.
:Trace: ``python/imas_core/_al_lowlevel.pyx:596-659, 867-911``.

Arrays of structures and plugins
--------------------------------

.. _REQ-PY-050:

REQ-PY-050: Array of structures wrappers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_arraystruct_action(ctx, path, timebase, size)``
            shall return ``(0, aosctx, size)``. When reading, pass size 0
            and get the stored size back. ``al_iterate_over_arraystruct(aosctx,
            step)`` shall accept any integer step, including negative
            steps (see :doc:`aos`).
:Status: Implemented.
:Trace: ``python/imas_core/_al_lowlevel.pyx:1048-1089``. IMAS-Python lazy
        loading moves backwards with negative steps
        (``imas/backends/imas_core/al_context.py:224-226``).

.. _REQ-PY-051:

REQ-PY-051: Plugin wrappers
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The plugin functions of :doc:`plugins` shall be available with
            the same names. They always raise on error.
            ``al_setvalue_parameter_plugin(name, value, plugin)`` accepts
            the same value types as ``al_write_data``.
:Status: Implemented. IMAS-Python wraps them but never calls them.
:Trace: ``python/imas_core/_al_lowlevel.pyx:1094-1205``.

.. _REQ-PY-052:

REQ-PY-052: C declarations match the C header
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The Cython declarations shall match ``al_lowlevel.h``.
:Status: Defect (cosmetic). ``al_lowlevel_interface.pxd`` declares
         ``al_status_t.message`` as ``char[10]`` instead of ``char[256]``.
         The code compiles against the real header, so the layout is
         right, but the declaration is misleading.
:Trace: ``python/imas_core/al_lowlevel_interface.pxd:3-5``.
