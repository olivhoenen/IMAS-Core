URIs and backend selection
==========================

A data entry is identified by a URI, passed to ``al_begin_dataentry_action``.
The URI selects the backend and gives it the location of the data, either
directly (``path`` or ``mapping``) or through the legacy parameters
(``user``, ``database``, ``version``, ``pulse``, ``run``). This page
specifies how IMAS-Core parses and interprets the URI. The user-facing
description is in :doc:`../user_guide/uris_guide`, and the query keys of
each backend are listed in :doc:`../user_guide/backends_guide` and
:doc:`backends`.

The parser is in ``include/uri_parser.h``. The interpretation is in
``DataEntryContext`` (``src/al_context.cpp``).

Errors raised while the URI is interpreted are context errors
(``ALContextException``) or, for environment problems, backend errors
(``ALBackendException``). ``al_begin_dataentry_action`` currently reports
context errors as ``UNKNOWN_ERR``; see :ref:`REQ-ERR-003`.

Syntax
------

.. _REQ-URI-001:

REQ-URI-001: URI structure
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall accept URIs of the form
            ``scheme:[//authority]path[?query][#fragment]``, where the
            authority is ``[userinfo@]host[:port]``.
:Nominal: ``imas:hdf5?path=/data/entry`` gives scheme ``imas``, no
          authority, path ``hdf5`` and query ``path=/data/entry``.
          ``imas://uda.iter.org:56565/uda?path=/x;backend=hdf5`` gives host
          ``uda.iter.org``, port ``56565`` and path ``/uda``.
:Limits: The scheme may contain letters, digits, ``+``, ``.`` and ``-``,
         and is converted to lower case: ``IMAS:hdf5?path=x`` is
         accepted. A bracketed IPv6 host (``[::1]``) is kept as is, and its
         port is not parsed.
:Errors: No ``:`` in the URI, an empty scheme or a scheme with another
         character fails with the context error
         ``Unable to parse the URI: <uri>``. A port that is not a decimal
         number fails with the same error. No handle is allocated and no
         file is created.
:Status: Implemented.
:Trace: ``include/uri_parser.h`` (``parse_scheme``, ``parse_authority``,
        ``parse_uri``); ``src/al_context.cpp:51-53``.

.. _REQ-URI-002:

REQ-URI-002: URI scheme
^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The scheme of a data entry URI shall be ``imas``.
:Nominal: ``imas:memory?path=a`` opens a Memory data entry.
:Limits: Upper-case ``IMAS`` is accepted (see :ref:`REQ-URI-001`).
:Errors: Not defined.
:Status: Unspecified. The scheme is parsed but never checked:
         ``file:hdf5?path=/x`` opens the same HDF5 entry as
         ``imas:hdf5?path=/x``. It is not settled whether another scheme
         should be refused.
:Trace: ``src/al_context.cpp`` (no use of ``uri.scheme`` other than
        copying it).

.. _REQ-URI-003:

REQ-URI-003: End of the authority
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The authority shall end at the first ``/``, ``?`` or ``#``
            after ``//``, so that a URI without a path after the host is
            still split correctly.
:Nominal: ``imas://host:1234/uda?path=/a`` gives host ``host`` and port
          ``1234``.
:Limits: ``imas://host`` (nothing after the host) gives host ``host`` and
         an empty path.
:Errors: See :ref:`REQ-URI-001` for the port.
:Status: Defect. The parser only looks for ``/``. In
         ``imas://host?path=/work/x``, the authority runs up to the first
         ``/`` inside the query value, so the host becomes
         ``host?path=`` and the rest of the URI is lost. When there is no
         ``/`` at all, the remainder is computed from an overflowed
         position. These URIs select the UDA backend (they have a host) with
         a wrong host name.
:Trace: ``include/uri_parser.h:269-278``.

.. _REQ-URI-004:

REQ-URI-004: Query syntax
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The query shall be a list of ``key=value`` arguments separated
            by ``;``, ``&`` or ``?``. Keys are case-sensitive. A key
            without ``=`` shall be present with an empty value. Only the
            first ``=`` of an argument separates the key from the value.
:Nominal: ``path=/a;hdf5_compression=no`` and ``path=/a&hdf5_compression=no``
          are equivalent.
:Limits: An empty argument (``a=1;;b=2``) adds a key with an empty name and
         no value, which is ignored. ``path=a=b`` gives ``path`` the value
         ``a=b``. ``?`` inside the query is a separator, so a value cannot
         contain ``?``, ``;``, ``&`` or ``#``; there is no percent-decoding.
         Keys that no component reads are ignored without a warning, as
         documented in :doc:`../user_guide/backends_guide`.
:Status: Implemented.
:Trace: ``include/uri_parser.h:313-339`` (``parse_query``),
        ``QueryDict::insert``. IMAS-Python splits the query the same way
        (``imas/backends/imas_core/db_entry_al.py:72-77``).

.. _REQ-URI-005:

REQ-URI-005: Repeated query keys
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The meaning of a key given more than once is not defined yet.
:Nominal: Not applicable.
:Limits: Currently the distinct values are kept in a hash set and returned
         joined with ``;`` in an unspecified order: ``path=/a;path=/b``
         gives ``path`` the value ``/a;/b`` or ``/b;/a``. The same value
         given twice is kept once.
:Status: Unspecified. Options: refuse the URI, keep the last value, or keep
         the current behavior for keys that accept lists (UDA
         ``init_args``).
:Trace: ``include/uri_parser.h:89-97, 146-157``.

.. _REQ-URI-006:

REQ-URI-006: Fragment
^^^^^^^^^^^^^^^^^^^^^

:Statement: A fragment (after ``#``) shall be parsed, kept apart from the
            query and otherwise ignored, because no backend uses it.
:Nominal: ``imas:hdf5?path=/a#equilibrium`` gives fragment
          ``equilibrium``.
:Limits: Without ``#``, the fragment shall be empty.
:Status: Defect, cosmetic. Without ``#``, the fragment is set to the whole
         query string, so the URI printed by ``al_context_info`` and in
         read warnings ends with ``#<query>``. With ``#`` but no ``?``
         (``imas:hdf5#x``), the fragment is also parsed as a query key.
:Trace: ``include/uri_parser.h:303-311`` and ``:338``
        (``uri.substr(npos + 1)`` is the whole string).

Backend selection
-----------------

.. _REQ-URI-010:

REQ-URI-010: Backend selected from the URI path
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall select the backend from the URI path as
            follows, and from nothing else.

            .. list-table::
               :header-rows: 1
               :widths: 40 30

               * - URI path
                 - Backend
               * - ``hdf5``
                 - HDF5
               * - ``mdsplus``
                 - MDSplus
               * - ``memory``
                 - Memory
               * - ``ascii``
                 - ASCII
               * - ``flexbuffers``
                 - Flexbuffers
               * - ``uda``, or any path when the URI has a host
                 - UDA
:Nominal: ``imas:hdf5?path=/a`` selects HDF5;
          ``imas://server/uda?path=/a;backend=hdf5`` selects UDA.
:Limits: Names are case-sensitive: ``imas:HDF5?path=/a`` is refused. A
         host takes precedence over the path: ``imas://server/hdf5?path=/a``
         selects UDA. A leading ``/`` without a host (``imas:/hdf5``) is not
         recognized. No URI selects ``NO_BACKEND``.
:Errors: Any other path, including an empty one, fails with the context
         error ``Unable to identify a backend from the URI``, reported as
         ``UNKNOWN_ERR``, for every open mode. No handle is returned (the
         output handle is not written) and nothing is created on disk.
:Status: Implemented.
:Trace: ``src/al_context.cpp:205-220`` (``setBackendID``). Tests
        ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``,
        ``BackendFactory.EmptyBackendNameInUriReturnsCleanError``,
        ``BackendFactory.UnknownBackendNameIsRefusedForEveryOpenMode``
        (PR #98).

.. _REQ-URI-011:

REQ-URI-011: Backend not built
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the selected backend was not compiled into the library,
            opening shall fail with ``BACKEND_ERR`` and the message
            ``<Name> backend is not available within current install``.
:Nominal: A build without MDSplus refuses ``imas:mdsplus?path=/a``.
:Limits: Memory and Flexbuffers are always built. HDF5, MDSplus, UDA and
         ASCII depend on build options.
:Errors: As stated. No handle is allocated.
:Status: Implemented. IMAS-Python detects a missing MDSplus backend by
         catching ``ImasCoreBackendException`` and looking for
         ``not available`` in its message, so this wording is part of the
         interface.
:Trace: ``src/al_backend.cpp:26-92`` (``Backend::initBackend``).
        IMAS-Python ``imas/test/conftest.py:61-67``.

.. _REQ-URI-012:

REQ-URI-012: Location given by path or mapping
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A URI shall give the data location with exactly one of the
            ``path`` and ``mapping`` keys, or with the legacy keys of
            :ref:`REQ-URI-020`. ``mapping`` is used by UDA mapping plugins;
            all other backends use ``path``.
:Nominal: ``imas:hdf5?path=/data/entry``.
:Errors: Both ``path`` and ``mapping`` fail with the context error
         ``URI cannot contain both path and mapping query arguments``
         (reported as ``UNKNOWN_ERR``).
:Status: Implemented.
:Trace: ``src/al_context.cpp:56-66``.

.. _REQ-URI-013:

REQ-URI-013: Local redirection of UDA URIs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the URI path is ``uda`` or ``/uda``, its host is listed in
            the environment variable ``IMAS_LOCAL_HOSTS`` (a
            ``;``-separated list) and the query has a ``backend`` key,
            IMAS-Core shall drop the host and the ``backend`` key and open
            the data entry with the named local backend, as if the URI were
            ``imas:<backend>?<other keys>``.
:Nominal: With ``IMAS_LOCAL_HOSTS=uda.iter.org``,
          ``imas://uda.iter.org:56565/uda?path=/a;backend=hdf5`` opens
          ``/a`` with the local HDF5 backend.
:Limits: The host is compared exactly, without the port. Without a
         ``backend`` key, the URI stays a UDA URI. Empty items in the list
         are ignored.
:Errors: An unknown ``backend`` value fails as in :ref:`REQ-URI-010`.
:Status: Implemented.
:Trace: ``src/al_context.cpp:186-203`` (``checkUriHost``);
        :doc:`../user_guide/configuration`.

Legacy parameters
-----------------

.. _REQ-URI-020:

REQ-URI-020: Legacy URI resolution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the URI has neither ``path`` nor ``mapping``, IMAS-Core
            shall require the keys ``user``, ``database``, ``version``,
            ``pulse`` (or its alias ``shot``) and ``run``, and shall build
            the path as follows:

            - ``user=public``:
              ``$IMAS_HOME/shared/imasdb/<database>/<version>/<pulse>/<run>``
            - ``user`` starting with ``/``:
              ``<user>/<database>/<version>/<pulse>/<run>``
            - any other ``user``:
              ``<home of user>/public/imasdb/<database>/<version>/<pulse>/<run>``,
              where the home directory comes from the account database
              (on Windows, the Documents folder of the current user).
:Nominal: ``imas:hdf5?user=public;database=ITER;version=3;pulse=131024;run=41``
          opens ``$IMAS_HOME/shared/imasdb/ITER/3/131024/41``.
:Limits: The values are used as text and not validated: a non-numeric
         ``pulse`` is accepted as a directory name.
:Errors: All are reported by ``al_begin_dataentry_action`` and no handle is
         allocated.

         - A missing key fails with the context error
           ``'<key>' is not specified in URI but it is required when path is
           not specified (legacy mode)``.
         - Both ``pulse`` and ``shot`` fail with the context error
           ``Can't provide both 'pulse and 'shot', use just 'pulse'
           instead``.
         - ``user=public`` without ``IMAS_HOME`` fails with ``BACKEND_ERR``
           ``when user is 'public', IMAS_HOME environment variable should be
           set.``
         - An unknown user fails with ``BACKEND_ERR``
           ``Can't find or access <user> user's data``.
:Status: Implemented.
:Trace: ``src/al_context.cpp:102-184`` (``buildURIFromLegacy``).

.. _REQ-URI-021:

REQ-URI-021: Other keys kept in legacy mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In legacy mode, the query keys other than the legacy keys shall
            be kept and passed to the backend, as in path mode.
:Nominal: ``imas:hdf5?user=u;database=d;version=3;pulse=1;run=1;hdf5_compression=no``
          shall disable compression.
:Status: Defect, wrong result. The URI is rebuilt with the ``path`` key
         only, so every other key (``hdf5_*``, UDA ``backend``,
         ``cache_mode``, ``verbose``, ...) and the fragment are silently
         dropped. URIs built by
         ``al_build_uri_from_legacy_parameters`` with an ``options``
         argument lose those options in the same way.
:Trace: ``src/al_context.cpp:181-183``.

.. _REQ-URI-022:

REQ-URI-022: Building a URI from legacy parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_build_uri_from_legacy_parameters(backendID, pulse, run,
            user, tokamak, version, options, &uri)`` shall return the URI
            ``imas:<backend>?user=<user>;pulse=<pulse>;run=<run>;database=<tokamak>;version=<major>``,
            followed by ``;<options>`` when ``options`` is not empty. In
            the C API the caller frees ``uri`` with ``free``.
:Nominal: ``(HDF5_BACKEND, 1, 2, "u", "ITER", "3.39.0", "")`` gives
          ``imas:hdf5?user=u;pulse=1;run=2;database=ITER;version=3``.
:Limits: The backend ids accepted are ``MDSPLUS_BACKEND``,
         ``HDF5_BACKEND``, ``ASCII_BACKEND``, ``MEMORY_BACKEND`` and
         ``UDA_BACKEND``. A ``NULL`` ``options`` is treated as empty.
:Errors: Another backend id fails with ``LOWLEVEL_ERR`` and the message
         ``getURIBackend, converting backend ID to backend URI string not
         yet implemented``; no URI is returned.
:Status: Implemented, apart from :ref:`REQ-URI-023`, :ref:`REQ-URI-024`
         and :ref:`REQ-URI-025`.
:Trace: ``src/al_context.cpp:222-283``; ``src/al_lowlevel.cpp:1349-1414``.
        Tests ``UriOwnership.CallerFreesBuiltUri``,
        ``BackendFactory.UnknownPositiveLegacyBackendIdIsRefused``
        (PR #95, #98). IMAS-Python
        ``imas/backends/imas_core/db_entry_al.py:105``, which passes the
        full Data Dictionary version (e.g. ``3.39.0``).

.. _REQ-URI-023:

REQ-URI-023: Major version in built URIs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``version`` written into a built URI shall be the text
            before the first ``.`` of the ``version`` argument.
:Nominal: ``3.39.0`` gives ``3``; ``4`` gives ``4``.
:Limits: ``10.0`` shall give ``10``.
:Errors: An empty ``version`` shall be refused with ``LOWLEVEL_ERR``.
:Status: Defect, wrong result. Only the first character is used: ``10.0``
         gives ``1``, and an empty string writes a NUL character into the
         URI, which then ends the C string there.
:Trace: ``src/al_context.cpp:234, 252``.

.. _REQ-URI-024:

REQ-URI-024: Flexbuffers in built URIs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_build_uri_from_legacy_parameters`` shall accept
            ``FLEXBUFFERS_BACKEND`` and produce ``imas:flexbuffers?...``.
:Status: Defect. It fails with ``LOWLEVEL_ERR`` because
         ``getURIBackend`` has no case for it.
:Trace: ``src/al_context.cpp:263-283``. Tests
        ``RoundTripKnownDefects.DISABLED_BuildUriSupportsFlexbuffers`` and
        tripwire ``RoundTripKnownDefects.BuildUriFlexbuffersCurrentlyFails``
        (PR #95).

.. _REQ-URI-025:

REQ-URI-025: Argument checks in the C builder
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The C function ``al_build_uri_from_legacy_parameters`` shall
            accept ``options`` of any length and shall fail with
            ``LOWLEVEL_ERR``, not crash, when ``user``, ``tokamak`` or
            ``version`` is ``NULL``.
:Status: Defect, crash. ``options`` is copied with ``strcpy`` into a
         1024-byte stack buffer, so a longer string overflows it. ``NULL``
         ``user``, ``tokamak`` or ``version`` is dereferenced.
:Trace: ``src/al_lowlevel.cpp:1389-1395``; ``src/al_context.cpp:241-262``.
