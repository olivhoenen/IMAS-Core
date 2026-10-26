Queries and introspection
=========================

This page specifies the functions that describe a data entry or the library
itself, without reading IDS data: listing occurrences and filled paths,
describing a context, and the version and name lookups.

The handle rules are in :doc:`contexts`. The status codes are in
:doc:`errors`.

Data entry queries
------------------

.. _REQ-QRY-001:

REQ-QRY-001: Listing IDS occurrences
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_get_occurrences(pctx, ids_name, &list, &size)`` shall
            return, in increasing order, the numbers of the occurrences of
            IDS ``ids_name`` that exist in the data entry of ``pctx``.
            ``ids_name`` is the bare IDS name, without an occurrence.
            ``list`` is allocated with ``malloc`` and released by the caller
            with ``free``.
:Nominal: With occurrences 0, 1 and 3 written, the list is ``[0, 1, 3]``.
:Limits: - When no occurrence exists, ``size == 0``. ``list`` may then be
           left unset (NoBackend) or set to a ``malloc(0)`` pointer (HDF5,
           ASCII, MDSplus). The caller shall free ``list`` only when
           ``size > 0`` and shall otherwise ignore it.
         - The Python binding returns ``None`` for an empty list.
           IMAS-Python turns it into ``[]``.
:Errors: - Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in
           store".
         - Backend without this feature: ``BACKEND_ERR`` (Memory,
           Flexbuffers: "... not implemented ...").
         - Data entry not open: ``BACKEND_ERR`` (HDF5 "master file not
           opened", MDSplus "Data entry not open").
:Status: *Divergence*. Each backend decides differently whether an
         occurrence "exists":

         - **HDF5:** the master file has a link named ``<ids>`` (occurrence
           0) or ``<ids>_<N>``. An occurrence emptied leaf by leaf is still
           listed (:ref:`REQ-DATA-030`).
         - **MDSplus:** the occurrence's ``ids_properties/homogeneous_time``
           is set.
         - **ASCII:** a file ``<ids>.ids`` or ``<ids><N>.ids`` exists.
         - **NoBackend:** always an empty list.

         *Unspecified*: the core does not check that ``pctx`` is a data
         entry context.
:Trace: ``src/al_lowlevel.cpp:1753-1776``;
        ``src/hdf5/hdf5_reader.cpp:1526-1590``;
        ``src/mdsplus/mdsplus_backend.cpp:4887-4954``;
        ``src/ascii_backend.cpp:684-709``; ``src/memory_backend.cpp:1145-1149``;
        ``src/no_backend.cpp:80-86``; ``python/imas_core/_al_lowlevel.pyx:1207-1224``.
        IMAS-Python ``imas/backends/imas_core/al_context.py:168-175``,
        ``imas/test/test_list_occurrences.py``. Contract tests
        ``Occurrences.*``.

.. _REQ-QRY-002:

REQ-QRY-002: Listing filled paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_list_filled_paths(pctx, dataobjectname, &paths, &size)``
            shall return the paths of all nodes that hold data in the IDS
            occurrence ``dataobjectname`` (``<ids>`` or ``<ids>/<N>``).
            Paths are ``/``-separated, relative to the IDS root, without AoS
            indices, and listed once whatever the number of AoS elements.
            Each string and the array are allocated with ``malloc`` and
            released by the caller with ``free``.
:Nominal: For an IDS with ``ids_properties/homogeneous_time`` and
          ``time_slice[:]/global_quantities/ip`` written, the list includes
          ``ids_properties/homogeneous_time`` and
          ``time_slice/global_quantities/ip``.
:Limits: - The order of the list is not specified.
         - Internal bookkeeping entries (HDF5 ``_SHAPE`` datasets) are not
           listed.
         - An IDS occurrence that does not exist gives ``size == 0``, and
           ``paths`` is left unset. The caller shall free ``paths`` only
           when ``size > 0``.
         - IMAS-Python raises ``DataEntryException`` for an empty list.
:Errors: - Backend without this feature: ``BACKEND_ERR`` (Memory, ASCII,
           MDSplus, Flexbuffers: "... not implemented ...").
         - UDA with a ``backend`` other than ``hdf5``: an ``ALException``
           "UDABackend only supports HDF5 backend for list_filled_paths
           API", reported as ``UNKNOWN_ERR`` because it is not a backend
           exception.
:Status: Implemented for HDF5, and for UDA with ``backend=hdf5``. The UDA
         error code is a *Defect*: it should be ``BACKEND_ERR``.
:Trace: ``src/al_lowlevel.cpp:1778-1801``;
        ``src/hdf5/hdf5_reader.cpp:1592-1630``;
        ``src/uda/uda_backend.cpp:1363-1380``. IMAS-Python
        ``imas/backends/imas_core/db_entry_al.py:367-377``,
        ``imas/test/test_list_filled_paths.py``. Contract test
        ``Backends/CapabilityMatrix.ListFilledPathsPositiveOrRefused``.

Context introspection
---------------------

.. _REQ-QRY-010:

REQ-QRY-010: Describing a context
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_context_info(ctx, &info)`` shall return a human-readable
            description of the context ``ctx`` in a string allocated with
            ``malloc``, which the caller releases with ``free``. The text
            gives the context type and its parameters (URI, IDS name,
            access and range mode, AoS path and index...).
:Nominal: A data entry context describes its URI. An operation context
          describes its IDS and access mode. An AoS context describes its
          path and current index.
:Limits: - ``ctx == 0`` returns ``"NULL context"`` with status 0.
         - The format is for humans and not stable: it contains a memory
           address, for example.
:Errors: Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in store".
         ``info`` is then an empty string, which the caller still frees.
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:801-835``. Contract tests ``ContextInfo.*``.

.. _REQ-QRY-011:

REQ-QRY-011: Backend of a data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_get_backendID(pctx, &id)`` shall return the backend
            identifier of the data entry context ``pctx``: one of
            ``ASCII_BACKEND`` (11), ``MDSPLUS_BACKEND`` (12),
            ``HDF5_BACKEND`` (13), ``MEMORY_BACKEND`` (14), ``UDA_BACKEND``
            (15) or ``FLEXBUFFERS_BACKEND`` (16). Any other context type
            shall be refused with ``LOWLEVEL_ERR`` "Wrong Context type
            stored".
:Errors: Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in store".
:Status: *Defect*: the context is ``static_cast`` to a data entry context
         without a check. A non-pulse handle returns an undefined value with
         status 0.
:Trace: ``src/al_lowlevel.cpp:838-854``. Contract tests
        ``GetBackendId.ReturnsTheBackendUsedToOpen``,
        ``GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError``.

Library introspection
---------------------

.. _REQ-QRY-020:

REQ-QRY-020: Library version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``getALVersion()`` shall return the version of the IMAS-Core
            library as a static string (for example ``5.7.2``, or a
            ``git describe`` string for development builds). The Python
            binding exposes it as ``get_al_version()``.
:Nominal: IMAS-Python parses it to detect features: ``al_get_occurrences``
          from 5.1, Flexbuffers from 5.3, ``al_begin_timerange_action``
          from 5.4, UDA ``cache_mode=none`` from 5.5.1, and
          ``al_list_filled_paths`` from 5.7. It also writes it into
          ``ids_properties/version_put/access_layer``.
:Limits: The caller shall not free the string.
:Status: Implemented.
:Trace: ``src/al_const.cpp:22-25``; ``python/imas_core/_al_lowlevel.pyx:1227-1229``.
        IMAS-Python ``imas/backends/imas_core/imas_interface.py:68-80``.

.. _REQ-QRY-021:

REQ-QRY-021: No Data Dictionary version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall not depend on a Data Dictionary version.
            ``getDDVersion()`` and the ``DD_VERSION`` constant shall return
            ``"!!DEPRECATED!!"``. The node
            ``ids_properties/version_put/data_dictionary`` shall be stored
            and read back like any other string, without being
            interpreted.
:Limits: UDA uses a DD XML file (``IDSDEF_PATH`` or ``IMAS_PREFIX``) to map
         paths. MDSplus uses a model tree built for one DD version. See
         :doc:`backends`.
:Status: Implemented.
:Trace: ``src/al_const.cpp:27-30``; ``include/al_defs.h.in:57``. Contract
        tests ``Introspection.GetDDVersionReturnsDeprecatedSentinel``,
        ``Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted``;
        ``python/tests/test_imasdef.py``.

.. _REQ-QRY-022:

REQ-QRY-022: Names of constants and error codes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``const2str(id)`` shall return the symbolic name of a constant
            (for example ``"READ_OP"``), and ``err2str(code)`` the name of
            an error code (for example ``"CONTEXT_ERR"``). Each uses its own
            table, and both return ``""`` for an identifier they do not
            know. The caller shall not free the result.
:Limits: ``TIMERANGE_OP`` and ``FLEXBUFFERS_BACKEND`` are defined
         constants, but ``const2str`` returns ``""`` for them. Messages that
         use ``const2str`` then show an empty name.
:Status: Implemented for unknown identifiers. *Unspecified*: whether the
         two missing names should be added (the contract tests currently
         pin ``""``).
:Trace: ``src/al_const.cpp:4-20``; ``include/al_const.h:144-174``. Contract
        tests ``Introspection.*``.
