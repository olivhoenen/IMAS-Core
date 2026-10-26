Backends
========

This page specifies how each storage backend behaves. The behavior shared by
all backends (URI parsing, open modes, version checks, read and write rules)
is specified in :doc:`uri`, :doc:`data_entry`, :doc:`operations` and
:doc:`data_access`. Here, each backend is described only where it adds
options, limits or errors, or where it differs from the shared behavior.

The URI query keys of each backend are listed in the user guide
(:doc:`../user_guide/backends_guide`, section :ref:`Query keys`). This page
specifies what happens with those keys, not what they are for.

Requirements are numbered in blocks: ``REQ-BE-001`` to ``009`` apply to all
backends, then ``010`` HDF5, ``030`` MDSplus, ``050`` UDA, ``070`` Memory,
``080`` ASCII, ``090`` Flexbuffers and ``100`` NoBackend.

Capability matrix
-----------------

The matrix shows the baseline behavior, as pinned by the contract test suite
(``tests/contract/``, pull requests #95 to #99). "Refused" means the call
returns the given error status; "no-op" means the call succeeds but has no
effect. A cell marked *defect* does not meet the requirement it links to.

.. list-table:: Operations per backend (baseline)
   :header-rows: 1
   :stub-columns: 1
   :widths: 22 13 13 13 13 13 13

   * - Operation
     - HDF5
     - MDSplus
     - UDA (remote)
     - Memory
     - ASCII
     - Flexbuffers
   * - Persistent storage
     - Yes
     - Yes
     - Yes (server)
     - No, process only
     - Yes
     - No, one buffer
   * - ``OPEN`` / ``FORCE_OPEN`` / ``CREATE`` / ``FORCE_CREATE``
     - Yes
     - Yes
     - Yes
     - Yes
     - ``CREATE`` overwrites (*defect*, REQ-BE-080)
     - Mode only selects serialize or deserialize (REQ-BE-090)
   * - ``ERASE_PULSE``
     - Same as ``CLOSE`` (*defect*)
     - Same as ``CLOSE`` (*defect*)
     - Same as ``CLOSE`` (*defect*)
     - Same as ``CLOSE`` (*defect*)
     - Same as ``CLOSE`` (*defect*)
     - Clears the buffer
   * - Global read and write
     - Yes
     - Yes, real DD paths only (REQ-BE-032)
     - Read only (REQ-BE-054)
     - Yes
     - Yes
     - Write, then read ``<buffer>``
   * - Slice read
     - Yes
     - Yes, including whole structures
     - If the server plugin is newer than 1.4.0
     - Yes (REQ-BE-072)
     - ``BACKEND_ERR``
     - ``BACKEND_ERR``
   * - Slice write (append)
     - Only the last slice is kept (*defect*)
     - Yes; scalars crash (*defect*, REQ-BE-035)
     - No
     - Yes
     - ``BACKEND_ERR``
     - ``BACKEND_ERR``
   * - Time range read
     - Yes
     - ``LOWLEVEL_ERR``
     - If the server plugin is newer than 1.4.0; fails today (*defect*)
     - ``LOWLEVEL_ERR``
     - ``LOWLEVEL_ERR``
     - ``LOWLEVEL_ERR``
   * - Delete a leaf
     - Deletes the whole occurrence (*defect*, fixed by PR #101)
     - Yes
     - Refused by the server
     - Yes
     - No-op (*defect*)
     - No-op
   * - Delete a structure or the whole IDS
     - Yes
     - ``%TREE-W-NNF`` (*defect*)
     - Refused by the server
     - Structure yes; whole IDS no-op (*defect*)
     - No-op (*defect*)
     - No-op
   * - ``al_get_occurrences``
     - Yes
     - Yes
     - Yes
     - ``BACKEND_ERR``
     - Yes
     - ``BACKEND_ERR``
   * - ``al_list_filled_paths``
     - Yes
     - ``BACKEND_ERR``
     - Only with ``backend=hdf5``
     - ``BACKEND_ERR``
     - ``BACKEND_ERR``
     - ``BACKEND_ERR``
   * - Version checked at open
     - Yes, 1.0
     - Yes, 1.1
     - No (*defect*)
     - No, 0.0
     - No, 0.0
     - No, 1.0

The delete behavior is specified in :doc:`data_access`, the slice and time
range behavior in :doc:`time`, and the queries in :doc:`queries`.

Shared backend rules
--------------------

.. _REQ-BE-001:

REQ-BE-001: Backends not built in are reported
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When a URI selects a backend that was not built into the
            library, ``al_begin_dataentry_action`` shall fail with
            ``BACKEND_ERR`` and a message naming the backend. It shall not
            create any file.
:Nominal: A library built without MDSplus, given ``imas:mdsplus?path=x``,
          returns ``BACKEND_ERR`` with "MDSplus backend is not available
          within current install".
:Limits: Memory and Flexbuffers are always built. ASCII, HDF5, MDSplus and
         UDA depend on the CMake options.
:Errors: An identifier that matches no backend gives ``BACKEND_ERR`` "Wrong
         backend identifier N". This cannot happen from a URI, because URI
         parsing fails first (see :doc:`uri`).
:Status: Implemented.
:Trace: ``src/al_backend.cpp:27-92`` (``Backend::initBackend``).
        IMAS-Python detects a missing MDSplus backend by catching
        ``ImasCoreBackendException`` and looking for "not available" in the
        message (``imas/test/conftest.py:61-67``).

.. _REQ-BE-002:

REQ-BE-002: Capability flags match the behavior
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Each backend reports whether it supports slice operations
            (``supportsTimeDataInterpolation``) and time range operations
            (``supportsTimeRangeOperation``). The reports shall match what
            the backend actually does.
:Nominal: HDF5 reports both. MDSplus reports slices only. ASCII, Memory,
          Flexbuffers and NoBackend report neither.
:Limits: The UDA backend asks the server: both are supported when the
         server plugin reports a version newer than 1.4.0 (REQ-BE-056). In
         fetch mode, the local backend answers.
:Errors: A time range action on a backend that does not support it fails
         with ``LOWLEVEL_ERR`` "Selected backend does not support time range
         operations" (see :doc:`operations`).
:Status: Divergence. Memory reports no slice support but does handle slice
         reads and writes itself (REQ-BE-072). The core only checks the time
         range flag, so the mismatch is not visible to callers.
:Trace: ``src/hdf5/hdf5_backend.h:136-146``,
        ``src/mdsplus/mdsplus_backend.h:255-262``,
        ``src/memory_backend.h:678-688``,
        ``src/flexbuffers_backend.h:53-54``. Contract tests
        ``CapabilityMatrix.{SliceReadPositiveOrRefused,
        SliceWriteBeginPositiveOrRefused, TimeRangeReadPositiveOrRefused}``.

.. _REQ-BE-003:

REQ-BE-003: Backend version stamps
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Each persistent backend shall write its format version into a
            data entry when it creates it, and shall report that stored
            version when the data entry is opened. The compatibility rule
            applied to the two versions is specified in :doc:`data_entry`.
:Nominal: HDF5 writes ``1.0`` into the ``HDF5_BACKEND_VERSION`` attribute
          of ``master.h5``. MDSplus writes ``1.1`` into the tree nodes
          ``VERSION:BACK_MAJOR`` and ``VERSION:BACK_MINOR``.
:Limits: ASCII, Memory and UDA report version ``0.0`` for both the code
         and the stored data, and Flexbuffers reports ``1.0`` for both, so
         the shared check never refuses them. An MDSplus data entry without
         version nodes is reported as ``1.0``.
:Errors: See REQ-BE-011 (HDF5) and REQ-BE-031 (MDSplus).
:Status: Implemented for HDF5 and MDSplus. For UDA, see REQ-BE-055.
:Trace: ``src/hdf5/hdf5_backend.cpp:24-66``,
        ``src/mdsplus/mdsplus_backend.cpp:13-14, 43-55, 5403-5440``,
        ``src/ascii_backend.cpp:170-180``, ``src/memory_backend.cpp:78``,
        ``src/flexbuffers_backend.cpp:130``,
        ``src/uda/uda_backend.cpp:279-290``. Contract tests
        ``Hdf5VersionDrift.*``, ``MdsplusVersionDrift.*``.

.. _REQ-BE-004:

REQ-BE-004: Handling of backend query keys
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A backend shall ignore query keys it does not know. Query keys
            are case-sensitive. A known key with a value that is not valid
            should be reported as an error.
:Nominal: ``imas:hdf5?path=x;foo=bar`` opens normally.
:Limits: For the HDF5 options ``hdf5_compression``, ``hdf5_read_buffering``
         and ``hdf5_write_buffering``, only ``no`` and ``n`` turn the
         option off. Any other value, such as ``false`` or ``0``, leaves it
         on without a warning. For ``hdf5_debug``, only ``yes`` and ``y``
         turn it on. The cache sizes ``hdf5_read_cache`` and
         ``hdf5_write_cache`` are read with ``atof``, so a value that is
         not a number gives a cache of 0 MiB. For the UDA keys ``verbose``
         and ``fetch``, only ``1`` and ``true`` mean "on".
:Errors: UDA refuses a ``cache_mode`` that is not ``none``, ``ids`` or
         ``struct`` (REQ-BE-052).
:Status: Unspecified. Invalid values are silently replaced by defaults on
         HDF5 but refused on UDA.
:Trace: ``src/hdf5/hdf5_utils.cpp:758-809`` (``HDF5Utils::readOptions``),
        ``src/uda/uda_backend.cpp:292-325``. User guide
        :ref:`Query keys`.

HDF5 backend
------------

See also :ref:`hdf5 backend` in the user guide.

.. _REQ-BE-010:

REQ-BE-010: HDF5 data entry layout and open modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An HDF5 data entry shall be a directory holding a
            ``master.h5`` file plus one file per stored IDS occurrence. The
            master file links to each occurrence file. The open modes shall
            behave as follows:

            - ``OPEN_PULSE`` opens an existing data entry and fails if
              ``master.h5`` is missing.
            - ``FORCE_OPEN_PULSE`` opens the data entry, creating it if it
              is missing.
            - ``CREATE_PULSE`` creates the data entry and fails if
              ``master.h5`` already exists.
            - ``FORCE_CREATE_PULSE`` removes the master file and all IDS
              files, then creates a new data entry.
:Nominal: ``FORCE_CREATE_PULSE`` on a directory with old data gives an
          empty data entry. Occurrence ``N`` of IDS ``x`` is stored in
          ``x_N.h5`` (``x.h5`` for occurrence 0) and linked from
          ``master.h5`` (see :doc:`queries`).
:Limits: The directory is created if needed.
:Errors: All errors below return ``BACKEND_ERR``:

         - ``OPEN_PULSE`` on a missing data entry: "HDF5 master file not
           found: <path>".
         - ``CREATE_PULSE`` on an existing one: "HDF5 master file: <path>
           already exists. Use FORCE_CREATE_PULSE instead."
         - The directory cannot be created: "Unable to create data-entry
           directory: <path>".
         - The open mode is not one of the four above: "Mode not yet
           supported".
:Status: Implemented.
:Trace: ``src/hdf5/hdf5_backend.cpp:72-100``,
        ``src/hdf5/hdf5_utils.cpp:150-163, 302-327, 455-465``. Contract
        tests ``Backends/DataEntryModes.*``.

.. _REQ-BE-011:

REQ-BE-011: HDF5 format version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The HDF5 backend shall open only files whose
            ``HDF5_BACKEND_VERSION`` attribute it can read. It shall refuse
            other files with an error that names the version found.
:Nominal: Files written by any IMAS-Core 5.x release carry ``1.0`` and
          open.
:Limits: The reader and writer factory accept exactly the string ``1.0``.
         This check runs before the shared major/minor rule of
         :doc:`data_entry`, so in practice the HDF5 backend is stricter
         than that rule.
:Errors: All errors below return ``BACKEND_ERR``:

         - The file has no version attribute: "Not a IMAS HDF5 pulse file.
           Unable to find attribute: HDF5_BACKEND_VERSION".
         - The stored version is anything other than ``1.0`` (for example
           ``999.0`` or ``1.999``): "No backend writer with version: X" or
           "No backend reader with version: X".
         - The version string has no number: "Unable to get backend
           version".
:Status: Implemented.
:Trace: ``src/hdf5/hdf5_utils.cpp:73-118``,
        ``src/hdf5/hdf5_backend_factory.cpp:15-35``,
        ``src/hdf5/hdf5_backend.cpp:36-66``. Contract tests
        ``Hdf5VersionDrift.*``.

.. _REQ-BE-012:

REQ-BE-012: HDF5 read-only files and concurrent writers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The HDF5 backend shall read a data entry whose files are
            read-only for the user. A write that needs a file the user
            cannot write shall fail without changing the file permissions.
:Nominal: Reading works when ``master.h5`` is writable and the IDS file is
          read-only, and when both are read-only. Writing an IDS whose link
          already exists in a read-only ``master.h5`` works if the IDS file
          is writable.
:Limits: The master file is first opened read-write and, if that fails,
         read-only.
:Errors: All errors below return ``BACKEND_ERR``:

         - Writing to a read-only IDS file: "Unable to open external file in
           Read-Write mode for IDS: <ids>. Permission denied."
         - An IDS file locked by another process: "... It might indicate
           that the file is being currently handled by a writing concurrent
           process."
         - Adding a new IDS when ``master.h5`` is read-only fails.
:Status: Implemented.
:Trace: ``src/hdf5/hdf5_utils.cpp:262-290, 312-327``.
        ``python/tests/test_hdf5_permissions.py`` (issue #40).

.. _REQ-BE-013:

REQ-BE-013: HDF5 data types and ranks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The HDF5 backend shall store and read back ``INTEGER_DATA``,
            ``DOUBLE_DATA`` and ``COMPLEX_DATA`` of rank 0 to 7, and
            ``CHAR_DATA`` of rank 0 (a single character), 1 (a string) and
            2 (a list of strings).
:Nominal: All 28 numeric type and rank cells round-trip unchanged. CHAR
          ranks 1 and 2 round-trip unchanged.
:Limits: CHAR data of rank 3 or more is refused at read time with a rank
         mismatch (``LOWLEVEL_ERR``). This is accepted, because IMAS models
         strings as rank 1 and string lists as rank 2.
:Errors: An unknown data type gives ``BACKEND_ERR`` "Data type not
         supported".
:Status: Defect. A CHAR scalar (rank 0) crashes the process instead of
         round-tripping.
:Trace: ``src/hdf5/hdf5_writer.cpp:463, 498``,
        ``src/hdf5/hdf5_dataset_handler.cpp:495-518``. Contract tests
        ``Backends/RoundTripMatrix.ReadEqualsWrite/HDF5_*``,
        ``RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips`` with
        tripwire ``RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes``.

.. _REQ-BE-014:

REQ-BE-014: HDF5 interpolation warning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When a slice read with ``LINEAR_INTERP`` cannot interpolate,
            the HDF5 backend shall print a warning to standard output,
            unless the environment variable
            ``IMAS_AL_DISABLE_INTERPOLATION_WARNING`` is set.
:Nominal: Without the variable, the warning "WARNING: Linear interpolation
          not possible..." is printed and the read still returns data.
:Limits: Any value of the variable, even an empty one, turns the warning
         off. The MDSplus backend reads the same variable.
:Status: Implemented. The variable is not listed in
         :doc:`../user_guide/configuration`.
:Trace: ``src/hdf5/hdf5_reader.cpp:23``,
        ``src/mdsplus/mdsplus_backend.cpp:4550``.

HDF5 behavior described on other pages:

- Repeated whole-IDS writes on an open data entry, and deletes with a path
  argument, are specified in :doc:`data_access` (pending PRs #100 and
  #101).
- Absolute field paths read from inside an array of structures are
  specified in :doc:`data_access` (pending PR #103).
- The append of slices is specified in :doc:`time`.

MDSplus backend
---------------

See also :ref:`mdsplus backend` in the user guide.

.. _REQ-BE-030:

REQ-BE-030: MDSplus data entry creation and open modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An MDSplus data entry shall be an MDSplus tree (shot 1 of the
            tree ``ids``) stored in the ``path`` directory. Creating a data
            entry shall copy the model tree found through the environment
            variable ``MDSPLUS_MODELS_PATH``. The open modes shall behave as
            for HDF5 (REQ-BE-010). A data entry exists when
            ``<path>/ids_001.tree`` exists.
:Nominal: ``FORCE_CREATE_PULSE`` creates the directory and a new tree from
          the model. ``OPEN_PULSE`` opens the tree.
:Limits: The query key ``readonly=1`` opens the tree read-only. The query
         key ``DEF_TREENAME`` replaces the tree name ``ids``. The backend
         looks for the tree through the process environment variable
         ``ids_path``, which it sets to ``<path>;$MDSPLUS_MODELS_PATH``
         while it opens the tree and then restores (see :doc:`nonfunctional`).
         ``FORCE_OPEN_PULSE`` on a missing data entry creates it, as
         ``CREATE_PULSE`` does.
:Errors: All errors below return ``BACKEND_ERR``:

         - ``OPEN_PULSE`` on a missing tree: the MDSplus error followed by
           the path. A hint is added when ``MDSPLUS_MODELS_PATH`` is not
           set.
         - ``CREATE_PULSE`` on an existing data entry: "<path>/ids_001.tree
           exists already, use a different mode to overwrite".
         - Creating without ``MDSPLUS_MODELS_PATH``: "MDSPLUS_MODELS_PATH is
           not set. Set it to the MDSplus models directory ...".
         - The directory cannot be created: "Unable to create data-entry
           directory: <path>".
         - Any other mode: "Mode not yet supported".
:Status: Implemented.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:4455-4552, 1486-1517``. Contract
        tests ``Backends/DataEntryModes.*/Mdsplus``. IMAS-Python builds the
        model tree itself when it creates an MDSplus data entry, and sets
        ``MDSPLUS_MODELS_PATH`` from imas_core 5.6.0
        (``imas/backends/imas_core/mdsplus_model.py:298-309``).

.. _REQ-BE-031:

REQ-BE-031: MDSplus format version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The MDSplus backend shall store its version (``1.1``) in a new
            data entry and refuse to open a data entry whose stored version
            it cannot handle.
:Nominal: A data entry created by the same version opens.
:Limits: A data entry without version nodes, made by an older Access Layer,
         is reported as ``1.0``. It opens for reading. A write action on
         it is refused, because the shared rule requires the minor versions
         to be equal when writing (see :doc:`data_entry`).
:Errors: All errors below return ``LOWLEVEL_ERR``:

         - A stored major version other than 1: "Compatibility between
           opened file version ... can't be ensured".
         - When the tree cannot be opened to read the version: "Cannot open
           MDSplus tree for getting version".
:Status: Unspecified. It is not settled whether a 1.1 backend should be
         allowed to write into a 1.0 data entry.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:13-14, 43-55, 5403-5440``,
        ``src/al_lowlevel.cpp:871-883, 956-966``. Contract tests
        ``MdsplusVersionDrift.{MatchingVersionOpensCleanly,
        MismatchedBackendVersionRefusesOpen}``.

.. _REQ-BE-032:

REQ-BE-032: MDSplus accepts only Data Dictionary paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The MDSplus backend shall store only nodes that exist in its
            model tree, which is generated from a Data Dictionary version.
            Writing any other path shall fail.
:Nominal: All Data Dictionary leaf types round-trip, including leaves
          nested up to three arrays of structures deep.
:Limits: Which type and rank combinations can be stored depends on the
         Data Dictionary. In DD 4.1.1 there are no CHAR ranks 3 to 7, no
         INTEGER ranks 4 to 7, no DOUBLE rank 7, and no COMPLEX ranks 0, 6
         or 7. A CHAR rank 0 read of a string node fails cleanly with a
         rank mismatch. Each IDS has as many occurrences as the
         Data Dictionary's ``maxoccur`` allows. Node names longer than 12
         characters are shortened inside the tree; callers do not see this.
:Errors: A path missing from the model fails with the MDSplus error
         ``%TREE-W-NNF, Node Not Found``. For timed writes and for writes
         inside an array of structures, the backend buffers the data, so
         the error only appears at ``al_end_action``.
:Status: Divergence. Other backends store any path. The contract suite
         skips its synthetic-path tests for MDSplus.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:784-890``. Contract tests
        ``Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/*``,
        ``Backends/AosMatrix.*/Mdsplus`` (skipped, divergence). IMAS-Python
        checks ``maxoccur`` itself before writing to MDSplus
        (``imas/backends/imas_core/db_entry_al.py:333``).

.. _REQ-BE-033:

REQ-BE-033: MDSplus time-dependent strings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The MDSplus backend shall refuse time-dependent string data.
:Errors: ``BACKEND_ERR`` "Time dependent string not supported. Use Arrays
         of structures instead".
:Status: Divergence.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:1877``.

.. _REQ-BE-034:

REQ-BE-034: MDSplus slices
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The MDSplus backend shall support slice reads with
            ``CLOSEST_INTERP``, ``PREVIOUS_INTERP`` and ``LINEAR_INTERP``,
            for leaves and for whole timed arrays of structures. It shall
            support slice appends, stored in segments. It does not support
            time range reads.
:Nominal: A slice read of a timed array of structures returns one element,
          chosen or interpolated as a whole. Elements appended one by one
          all persist.
:Limits: Data is stored in segments of 67072 bytes.
:Errors: ``al_begin_timerange_action`` returns ``LOWLEVEL_ERR``. An unknown
         interpolation mode returns ``BACKEND_ERR`` "Unsupported
         interpolation".
:Status: Implemented.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:19, 2461, 5442-5444``. Contract
        tests ``MdsplusStructAwareSliceInterpolation.*``,
        ``MdsplusSegmentBackedWrites.AppendedElementsAllPersist``,
        ``MdsplusTimebaseCache.SurvivesInterveningWritesWithoutStaleness``,
        ``MdsplusTimerangeResampling.RefusedWithNoUniqueSurface``.

.. _REQ-BE-035:

REQ-BE-035: MDSplus scalar write in a slice action
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Writing a rank 0 value inside a slice write action shall
            either succeed or return an error status. It shall not crash.
:Status: Defect. ``MDSplusBackend::writeData`` reads ``size[0]`` without
         checking it, and the size pointer is ``NULL`` for a scalar, so the
         process crashes.
:Trace: ``src/mdsplus/mdsplus_backend.cpp:4587``. Contract tests
        ``MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds``
        with tripwire
        ``MdsplusSliceWriteKnownDefectsDeath.ScalarWriteWithinSliceActionCurrentlyCrashes``.

MDSplus behavior described on other pages: deleting a structure or a whole
IDS (:doc:`data_access`), occurrences listed as ``<ids>/<N>`` and counted
only when ``ids_properties/homogeneous_time`` is set (:doc:`queries`), and
``ERASE_PULSE`` (:doc:`data_entry`).

UDA backend
-----------

See also :ref:`uda backend` in the user guide. The UDA backend forwards
each call to a UDA server, which reads data through a server-side plugin
(``IMAS`` by default). The contract tests run against UDA server 2.9.3, the
``IMAS`` UDA plugin 1.8.0 and DD 4.1.1.

.. _REQ-BE-050:

REQ-BE-050: UDA server and plugin selection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The UDA backend shall be selected by any URI with a host
            (``imas://host:port/uda?...``). It shall connect to that host
            and port. It shall use the server plugin named by the
            ``plugin`` query key, otherwise by the ``IMAS_UDA_PLUGIN``
            environment variable, otherwise ``IMAS``.
:Nominal: ``imas://uda.iter.org:56565/uda?path=/work/...;backend=hdf5``
          reads through the ``IMAS`` plugin, which uses the HDF5 backend on
          the server.
:Limits: The ``backend`` key defaults to ``mdsplus``, or to ``uda`` when
         ``mapping`` is given. ``init_args`` is passed to the plugin's
         ``init`` call, with ``;`` replaced by ``,``. With no host in the
         URI, the client defaults (``UDA_HOST``, ``UDA_PORT``) apply. The
         host is redirected to local access when it is listed in
         ``IMAS_LOCAL_HOSTS`` (see :doc:`uri`).
:Errors: A wrong port or unreachable server fails the open. An unknown
         plugin name fails at open.
:Status: Implemented. The capability query of REQ-BE-056 is sent before
         the ``plugin`` key is applied, so it always goes to the plugin
         from ``IMAS_UDA_PLUGIN`` or ``IMAS``.
:Trace: ``src/uda/uda_backend.cpp:233-238, 242-273, 466-492``. Contract
        tests ``UdaUniqueSurfaceTest.*`` (plugin, init_args, port).

.. _REQ-BE-051:

REQ-BE-051: UDA Data Dictionary loading
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The UDA backend shall load the Data Dictionary description
            (``IDSDef.xml``) from ``$IDSDEF_PATH``, or else from
            ``$IMAS_PREFIX/include/IDSDef.xml``. It shall refuse a Data
            Dictionary whose version does not match the data on the server.
:Nominal: With either variable pointing at a valid ``IDSDef.xml``, the
          backend opens. The ``dd_version`` query key overrides the version
          sent to the server.
:Errors: The errors below are raised as ``std::runtime_error`` and so return
         ``UNKNOWN_ERR``:

         - Neither variable is set: "neither IMAS_PREFIX or IDSDEF_PATH
           environmental variable is set".
         - The file is missing: "IDSDef.xml not found at either
           $IDSDEF_PATH or $IMAS_PREFIX/include/IDSDef.xml".

         A path missing from the Data Dictionary gives "cannot find node
         <path> in data dictionary".
:Status: Defect. A Data Dictionary of the wrong version is loaded without
         any check.
:Trace: ``src/uda/uda_xml.cpp:60-103``. Contract tests
        ``UdaUniqueSurfaceTest.{DdPresentLoadsAndOpenSucceeds,
        DdAbsentNeitherEnvVarSetFailsWithClearMessage,
        DdAbsentFileMissingAtIdsDefPathFailsWithClearMessage}``,
        ``UdaUniqueSurfaceTest.DISABLED_DdWrongVersionIsRejected``. IMAS-Python
        sets ``IDSDEF_PATH`` to the XML of the Data Dictionary version it
        uses (``imas/backends/imas_core/uda_support.py``).

.. _REQ-BE-052:

REQ-BE-052: UDA cache modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``cache_mode`` query key shall select how the client caches
            server data: ``none`` (no cache), ``ids`` (fetch a whole IDS,
            the default) or ``struct`` (fetch one structure at a time). The
            mode shall change only performance, never the values read.
:Nominal: The three modes return the same value for a leaf they can all
          reach.
:Limits: The cache is emptied when the data entry is closed. With
         ``cache_mode=ids``, a ``datapath`` given to
         ``al_begin_global_action`` limits the fetch to that subtree.
         Fields outside it read as absent. IMAS-Python uses this to read
         only ``ids_properties`` first.
:Errors: Any other value fails with "invalid cache mode". It is raised as a
         plain ``ALException``, so it returns ``UNKNOWN_ERR``.
:Status: Defect. In ``struct`` mode, a written top-level field reads back
         as the empty value.
:Trace: ``src/uda/uda_backend.cpp:292-325, 550-560, 927-955``. Contract
        tests ``UdaUniqueSurfaceTest.{InvalidCacheModeThrows,
        CacheModeNoneIdsStructAgreeForAosCoveredField,
        DatapathScopesCachePopulationFieldOutsideScopeReadsAsAbsent}``,
        ``UdaUniqueSurfaceTest.DISABLED_CacheModeStructPreservesTopLevelField``.
        IMAS-Python passes ``datapath="ids_properties"`` for UDA only
        (``imas/backends/imas_core/db_entry_al.py:190, 248``) and suggests
        ``cache_mode=none`` when the Data Dictionary versions differ (GH#97,
        ``db_entry_al.py:260-273``).

.. _REQ-BE-053:

REQ-BE-053: UDA fetch mode
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: With ``fetch=1`` or ``fetch=true``, the UDA backend shall
            download the data entry files into a local cache and then serve
            every call from a local backend of the type named by
            ``backend``.
:Nominal: The files are stored under ``<cache>/<path>``. ``<cache>`` is the
          ``local_cache`` key if given, otherwise
          ``<tmp>/uda-cache-of-$USER``, where ``<tmp>`` is the system
          temporary directory. Opening the same entry again reuses the
          cached files.
:Limits: Writes go to the local copy only. The server data does not
         change, and the local copy stays different from the server
         across later opens.
:Errors: Fetch mode without ``path`` fails with "No path provided in URI".
         Without ``backend`` it fails with "No backend provided in URI".
         Both return ``UNKNOWN_ERR``.
:Status: Divergence (local writes are accepted and kept).
:Trace: ``src/uda/uda_backend.cpp:410-445``. Contract tests
        ``UdaUniqueSurfaceTest.{FetchModeDownloadsHandsOffToLocalBackendAndReusesCacheOnReopen,
        FetchModeLocalCacheOptionOverridesDefaultCacheDir,
        FetchModeWriteDivergesFromServerAndStalePersistsAcrossReopen}``.

.. _REQ-BE-054:

REQ-BE-054: UDA remote access is read-only
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In remote mode, the UDA backend shall support reading only.
            ``al_write_data`` and ``al_delete_data`` shall return an error
            status.
:Nominal: Global reads, slice reads, occurrences and (with
          ``backend=hdf5``) filled paths work through the server. Reading
          an unwritten leaf of a stored IDS returns the empty value.
:Errors: Reading an IDS that was never written fails. ``al_delete_data``
         fails with the server message "Unknown function requested!".
:Status: Defect. ``al_write_data`` returns status 0 although nothing is
         stored. Also, a dynamic leaf inside an array of structures reads
         back as the empty value instead of the stored data.
:Trace: Contract tests
        ``UdaUniqueSurfaceTest.RemoteDeleteFailsWithUnknownFunctionRequested``,
        ``UdaUniqueSurfaceTest.DISABLED_RemoteWriteFailsWithDispatchError``,
        ``UdaAosKnownDefects.DISABLED_DynamicLeafInsideAosRoundTrips``,
        ``UdaBreadthTest.{ReadOfUnwrittenLeafOnSeededIdsReturnsHdf5AbsentSentinel,
        ReadFailsForNeverWrittenIds}``. ``src/uda/uda_backend.cpp:1379``
        (``list_filled_paths`` with other backends). The user guide lists
        UDA as read-only.

.. _REQ-BE-055:

REQ-BE-055: UDA version check
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In remote mode, opening a data entry shall apply the
            compatibility rule of :doc:`data_entry` to the format version
            stored on the server.
:Status: Defect. The client reports ``0.0`` for both its own version and the
         stored one, so its check never fails. A data entry with a version
         the server cannot read is refused only by the server's own error,
         forwarded as ``UNKNOWN_ERR``. In fetch mode, the local backend
         applies its own check.
:Trace: ``src/uda/uda_backend.h:37-38``,
        ``src/uda/uda_backend.cpp:279-290``. Contract test
        ``UdaUniqueSurfaceTest.DISABLED_OpenRefusesMismatchedStoredBackendVersion``.

.. _REQ-BE-056:

REQ-BE-056: UDA capability negotiation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The UDA backend shall support slice and time range reads when
            the server plugin's ``version()`` is newer than 1.4.0, and
            refuse them otherwise.
:Nominal: With plugin 1.8.0, ``al_begin_timerange_action`` returns status 0.
:Limits: The version query is sent when the backend is created, before the
         data entry is opened.
:Errors: The data entry cannot be opened, with ``UNKNOWN_ERR``, when the
         plugin returns no version string ("Invalid version returned from
         plugin") or a string that is not a semantic version.
:Status: Implemented. The boundary at 1.4.0 is not tested. Time range reads
         currently fail because the interpolation mode is never set (see
         :doc:`time`).
:Trace: ``src/uda/uda_backend.cpp:1190-1222``, ``src/al_backend.cpp:88-89``.
        Contract test
        ``UdaUniqueSurfaceTest.TimeRangeCapabilityGrantedByReferenceServerVersion180``.

.. _REQ-BE-057:

REQ-BE-057: UDA session release
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Closing a UDA data entry shall release every server-side handle
            opened for it.
:Status: Defect. Read requests send a ``uri=`` argument that is not
         stripped like the one sent by open and close. As a result, a
         server-side handle leaks and blocks a later local reopen of the
         same file.
:Trace: Contract test
        ``UdaUniqueSurfaceTest.DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle``.

.. _REQ-BE-058:

REQ-BE-058: UDA error classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Errors raised by the UDA backend should return ``BACKEND_ERR``,
            like those of the other backends.
:Status: Defect (minor). Most UDA errors are raised as a plain
         ``ALException`` or a standard exception, so they return
         ``UNKNOWN_ERR`` (see :doc:`errors`).
:Trace: ``src/uda/uda_backend.cpp`` (``throw ALException`` at lines 307,
        367, 417, 421, 1216 and others), ``src/uda/uda_xml.cpp:81, 92``.

Memory backend
--------------

See also :ref:`memory backend` in the user guide.

.. _REQ-BE-070:

REQ-BE-070: Memory store and open modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The Memory backend shall keep data in a store that belongs to
            the process and is keyed by the ``path`` query value. All data
            entries opened on the same ``path`` in a process shall share the
            same data. The open modes shall behave as for HDF5
            (REQ-BE-010).
:Nominal: Data written through one data entry can be read through a second
          data entry opened on the same path, for example from another
          language binding in the same program.
:Limits: Data is never written to disk. ``FORCE_CREATE_PULSE`` empties
         existing data.
:Errors: All errors below return ``BACKEND_ERR``:

         - ``CREATE_PULSE`` on an existing store: "CreatePulse: a pulse file
           already exists".
         - ``OPEN_PULSE`` on a missing store: "Missing pulse".
:Status: Implemented.
:Trace: ``src/memory_backend.h:451-497``. Contract tests
        ``Backends/DataEntryModes.*/Memory``.

.. _REQ-BE-071:

REQ-BE-071: Memory store lifetime
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The data of a Memory store shall remain available while at least
            one data entry context on its path is open. It shall be freed
            when the last of them is ended. Ending one data entry context
            shall never make another one invalid.
:Nominal: One data entry: the data is freed by ``al_end_action`` on the
          data entry context. ``al_close_pulse`` alone frees nothing.
:Status: Defect (crash). The backend counts how many data entries use a
         store, but its destructor removes and frees the store whatever the
         count is. When two data entries share a path, ending the first
         one leaves the second pointing at freed memory, and ending the
         second frees the store again (double free). The destructor also
         crashes when the data entry failed to open, because no store was
         attached.
:Trace: ``src/memory_backend.h:414-430, 465-467``. IMAS-Python works
        around this by not closing the second entry ("MEM backend already
        cleaned up, prevent SEGFAULT", ``imas/test/test_minimal_io.py:20``,
        ``imas/test/test_nbc_change.py:91``).

.. _REQ-BE-072:

REQ-BE-072: Memory slices
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The Memory backend shall support slice reads with
            ``CLOSEST_INTERP``, ``PREVIOUS_INTERP`` and ``LINEAR_INTERP``,
            and slice appends. It does not support time range reads.
:Nominal: Slices appended one by one can be read back one at a time. As
          on MDSplus, a slice read of a timed array of structures returns
          one element, chosen or interpolated as a whole.
:Limits: ``LINEAR_INTERP`` on CHAR data falls back to ``PREVIOUS_INTERP``.
:Errors: ``al_begin_timerange_action`` returns ``LOWLEVEL_ERR``. When the
         timebase is not a vector of doubles, the read fails with
         ``BACKEND_ERR`` "FATAL ERROR: time reference is not ...".
:Status: Divergence. Interpolation is done inside the backend, although the
         backend reports no slice support (REQ-BE-002).
:Trace: ``src/memory_backend.cpp:700-730, 1180-1200, 1464``. Contract tests
        ``CapabilityMatrix.SliceReadPositiveOrRefused/Memory``,
        ``CapabilityMatrix.SliceWriteBeginPositiveOrRefused/Memory``.

.. _REQ-BE-073:

REQ-BE-073: Memory queries
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_get_occurrences`` and ``al_list_filled_paths`` on a Memory
            data entry return ``BACKEND_ERR``.
:Errors: "get_occurrences() not implemented in MemoryBackend",
         "list_filled_paths() is not implemented in the MemoryBackend".
:Status: Unspecified. The data needed to answer both queries is in memory.
         It is not settled whether the Memory backend should implement
         them.
:Trace: ``src/memory_backend.cpp:1144-1154``. IMAS-Python skips its
        occurrence tests on Memory (``imas/test/test_list_occurrences.py``).

Deleting a whole IDS from a Memory store does nothing (*defect*, see
:doc:`data_access`). The Memory backend's internal lock does not work for
several threads (see :doc:`nonfunctional`).

ASCII backend
-------------

See also :ref:`ascii backend` in the user guide.

.. _REQ-BE-080:

REQ-BE-080: ASCII data entry layout and open modes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An ASCII data entry shall be a directory with one text file per
            IDS occurrence, named ``<ids><occurrence>.ids`` (``<ids>.ids``
            for occurrence 0). With the query key ``filename``, every IDS
            shall be written to and read from ``<path>/<filename>``. The
            open modes shall behave as for HDF5 (REQ-BE-010).
:Nominal: Writing occurrence 1 of ``equilibrium`` creates
          ``equilibrium1.ids``.
:Limits: With ``filename``, the file holds only the last IDS written,
         because each write action truncates it. IMAS-Python uses
         ``filename`` for ASCII serialization.
:Errors: All errors below return ``BACKEND_ERR``:

         - ``OPEN_PULSE`` on a missing file: "Unable to open data-entry,
           file does not exist: <file>".
         - ``OPEN_PULSE`` on a missing directory: "Unable to open
           data-entry, directory does not exist: <path>".
         - The directory cannot be created: "Unable to create data-entry
           directory: <path>".
:Status: Defect. ``CREATE_PULSE`` on an existing data entry succeeds and
         later writes overwrite it, instead of failing.
:Trace: ``src/ascii_backend.cpp:107-169``. Contract tests
        ``ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists``
        with tripwire ``ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently``.

.. _REQ-BE-081:

REQ-BE-081: ASCII operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ASCII backend shall support global ``READ_OP`` and
            ``WRITE_OP`` actions, one at a time per data entry. A write
            action replaces the whole IDS file.
:Limits: Slices and time ranges are not supported.
:Errors: All errors below return ``BACKEND_ERR``, except the time range
         case:

         - Slice action: "ASCII Backend does not support slice mode of
           operation!"
         - Time range action: ``LOWLEVEL_ERR``.
         - Another access mode: "Unsupported access mode for ASCII Backend!"
         - A second action while one is open: "Filename should be empty at
           this stage, but is <file>" or "IDS pulsefile <file> is already
           open".
         - The file cannot be opened: "Failed to open file <file> in read
           mode" (or "write mode").
:Status: Divergence.
:Trace: ``src/ascii_backend.cpp:184-246``. Contract tests
        ``CapabilityMatrix.*/ASCII``. IMAS-Python refuses lazy loading on
        ASCII (``imas/backends/imas_core/db_entry_al.py:178-179``).

.. _REQ-BE-082:

REQ-BE-082: ASCII data types and ranks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ASCII backend shall store and read back ``INTEGER_DATA``,
            ``DOUBLE_DATA`` and ``COMPLEX_DATA`` of rank 0 to 7, and
            ``CHAR_DATA`` of rank 0 to 2.
:Nominal: Numeric ranks 0 to 6 and CHAR ranks 0 to 2 round-trip.
:Errors: All errors below return ``BACKEND_ERR``:

         - CHAR data of rank 3 or more: "CHAR data > 2D is not implemented
           yet in ASCII Backend!"
         - Numeric data above rank 7: "<type> data > 7D is not implemented
           in ASCII Backend!"
         - Any other type: "Unsupported data type for ASCII Backend!"
:Status: Defect. At rank 7, ``INTEGER_DATA`` aborts the process, and
         ``DOUBLE_DATA`` and ``COMPLEX_DATA`` come back corrupted.
:Trace: ``src/ascii_backend.cpp:339, 367, 452, 529, 563, 637``. Contract
        tests ``RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips``.

.. _REQ-BE-083:

REQ-BE-083: ASCII arrays of structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Reading an array of structures from an ASCII file shall return
            its stored size, whatever was read before in the same action.
:Nominal: A high-level interface that reads fields in the order they were
          written (as IMAS-Python does) gets the right size.
:Status: Defect. The backend finds the array of structures by reading the
         next line of the file after the last field read. When the
         previous reads did not leave the cursor just before the array
         header, the size is reported as 0. This happens, for example,
         when the array of structures is the first thing read.
:Trace: ``src/ascii_backend.cpp:213-235, 656-682``. Contract tests
        ``AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize`` with
        tripwire ``AosKnownDefects.AsciiAosReadCurrentlyReportsZero``.

``al_delete_data`` does nothing on ASCII (*defect*, see :doc:`data_access`).

Flexbuffers backend
-------------------

See also :ref:`flexbuffers backend` in the user guide. Flexbuffers is a
serializer, not a store: it turns one IDS into a byte buffer and back.

.. _REQ-BE-090:

REQ-BE-090: Flexbuffers serialization protocol
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A Flexbuffers data entry opened with ``OPEN_PULSE`` shall
            deserialize. A data entry opened with any other mode shall
            serialize. The buffer is exchanged through the field name
            ``<buffer>``:

            - Serialization: after a ``WRITE_OP`` action that writes one
              IDS, reading ``<buffer>`` as ``CHAR_DATA`` of rank 1 returns
              the serialized bytes. The first byte is
              ``FLEXBUFFERS_SERIALIZER_PROTOCOL`` (61).
            - Deserialization: writing such bytes to ``<buffer>``, then
              starting a ``READ_OP`` action, makes the IDS readable.
:Nominal: IMAS-Python serializes with ``imas:flexbuffers?path=/``. The
          ``path`` value is not used, but a ``path`` key is needed for the
          URI to be accepted.
:Limits: ``<buffer>`` may be read or written on the data entry context.
         Plugins are never applied to ``<buffer>``. The buffer holds one
         IDS; each ``WRITE_OP`` action starts a new one.
:Errors: All errors below return ``BACKEND_ERR``:

         - A buffer whose first byte is not 61: "Serialize Backend: Unknown
           serializer protocol: N".
         - A buffer written on a machine with the other byte order: "Error
           when deserializing data: expected endian marker ...".
         - Writing ``<buffer>`` with vectors still open: "Incomplete
           vectors present".
         - Reading ``<buffer>`` while deserializing: "Reading <buffer>, but
           no builder available".
         - Writing a field while deserializing: "Writing data, but no
           builder available".
:Status: Implemented.
:Trace: ``src/flexbuffers_backend.cpp:130-200, 276-300, 336-385``.
        IMAS-Python ``imas/ids_toplevel.py:161-203``.

.. _REQ-BE-091:

REQ-BE-091: Flexbuffers restrictions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The Flexbuffers backend shall refuse operations a serializer
            cannot perform.
:Errors: All errors below return ``BACKEND_ERR``, except the time range
         case:

         - A slice action: "Serialize Backend does not support slice mode".
         - A time range action: ``LOWLEVEL_ERR``.
         - Reading a field while serializing: "Cannot read data when
           serializing."
         - Ending contexts in a different order than they were started:
           "Unexpected nesting of contexts".
         - ``al_get_occurrences`` and ``al_list_filled_paths``: "... is not
           implemented in the Serialize Backend".
         - An unsupported data type: "Unsupported data type for the
           Serialize Backend".

         ``al_delete_data`` does nothing.
:Status: Implemented.
:Trace: ``src/flexbuffers_backend.cpp:156, 225, 327, 358, 387-407``.
        Contract tests ``Backends/RoundTripMatrix.ReadEqualsWrite/Flexbuffers_*``
        (writes accepted, reads refused).

The legacy URI builder cannot produce a Flexbuffers URI (*defect*, see
:doc:`uri`).

NoBackend
---------

.. _REQ-BE-100:

REQ-BE-100: NoBackend
^^^^^^^^^^^^^^^^^^^^^

:Statement: ``NO_BACKEND`` (10) identifies a backend that stores nothing.
            Every operation succeeds and does nothing, and every read
            returns "no data".
:Limits: No URI selects it, and the legacy URI builder refuses it, so it
         cannot be reached through the public API.
:Status: Unspecified. It is not settled whether ``NO_BACKEND`` should be
         reachable or removed.
:Trace: ``src/no_backend.cpp``, ``src/al_backend.cpp:31-35``.
