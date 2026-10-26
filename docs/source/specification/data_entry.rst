Opening and closing data entries
================================

A data entry is opened with ``al_begin_dataentry_action(uri, mode,
&ctx)``, which returns a data entry handle. It is closed with
``al_close_pulse(ctx, mode)`` and its handle is released with
``al_end_action(ctx)``. This page specifies the open and close modes, the
backend format version checks and the life of a data entry. How the URI is
interpreted is in :doc:`uri`; how handles behave in general is in
:doc:`contexts`.

Opening
-------

.. _REQ-DE-001:

REQ-DE-001: Open a data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_dataentry_action(uri, mode, &ctx)`` shall parse the
            URI, create the backend, open or create the data entry
            according to ``mode``, and return status 0 with a new data
            entry handle (strictly positive) in ``ctx``.
:Nominal: ``al_begin_dataentry_action("imas:hdf5?path=/tmp/e",
          FORCE_CREATE_PULSE, &ctx)`` returns a handle that can be passed
          to the ``al_begin_*_action`` functions.
:Limits: Opening several data entries, or the same data entry several
         times, gives distinct handles (but see :ref:`REQ-DE-030`).
:Errors: URI errors are listed in :doc:`uri`. Mode errors are in
         :ref:`REQ-DE-006`. Storage errors (missing entry, permissions,
         corrupted files) are ``BACKEND_ERR`` with a backend-specific
         message. Version errors are in :ref:`REQ-DE-010`.
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:856-906``. IMAS-Python
        ``imas/backends/imas_core/db_entry_al.py:62``.

.. _REQ-DE-002:

REQ-DE-002: OPEN_PULSE
^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``OPEN_PULSE`` (40) shall open an existing data entry and shall
            fail if it does not exist.
:Nominal: Opening an entry created earlier succeeds and its IDSs can be
          read.
:Errors: An absent entry fails with ``BACKEND_ERR``. The messages are, per
         backend:

         - HDF5: ``HDF5 master file not found: <path>``.
         - Memory: ``Missing pulse``.
         - ASCII: ``Unable to open data-entry, directory does not exist:
           <path>``, or ``..., file does not exist: <path>/<filename>``
           when ``filename`` is given.
         - MDSplus: the MDSplus error followed by the path, with a hint when
           ``MDSPLUS_MODELS_PATH`` is not set.
:Status: Implemented. Divergence for Flexbuffers, which has no storage:
         ``OPEN_PULSE`` selects deserialization and never fails at open.
         IMAS-Python maps its mode ``"r"`` to ``OPEN_PULSE``.
:Trace: ``src/hdf5/hdf5_utils.cpp:304-309``, ``src/memory_backend.h:465-481``,
        ``src/ascii_backend.cpp:107-127``,
        ``src/mdsplus/mdsplus_backend.cpp:4490-4503``,
        ``src/flexbuffers_backend.cpp:134-146``. Tests
        ``DataEntryModes.OpenPulseFailsWhenAbsent``,
        ``DataEntryModes.OpenPulseSucceedsOncePresent`` (PR #95; MDSplus in
        PR #96).

.. _REQ-DE-003:

REQ-DE-003: FORCE_OPEN_PULSE
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``FORCE_OPEN_PULSE`` (41) shall open the data entry if it
            exists, and create an empty one otherwise.
:Nominal: On an absent entry it behaves as ``CREATE_PULSE``; on an
          existing entry as ``OPEN_PULSE`` (including the version check of
          :ref:`REQ-DE-010`).
:Status: Implemented. IMAS-Python maps its mode ``"a"`` to
         ``FORCE_OPEN_PULSE``.
:Trace: Test ``DataEntryModes.ForceOpenPulseCreatesWhenAbsent`` (PR #95).

.. _REQ-DE-004:

REQ-DE-004: CREATE_PULSE
^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``CREATE_PULSE`` (42) shall create a new, empty data entry and
            shall fail without modifying anything if the entry already
            exists.
:Nominal: Creating an entry at an unused location succeeds.
:Errors: An existing entry fails with ``BACKEND_ERR``:

         - HDF5: ``HDF5 master file: <path> already exists. Use
           FORCE_CREATE_PULSE instead.``
         - Memory: ``CreatePulse: a pulse file already exists``.
         - MDSplus: ``<path>/ids_001.tree exists already, use a different
           mode to overwrite``.
:Status: Defect for ASCII, data loss: ASCII has no existence check, so
         ``CREATE_PULSE`` succeeds on an existing entry, and the next write
         of an IDS overwrites its file. Implemented for the other backends.
         IMAS-Python maps its mode ``"x"`` to ``CREATE_PULSE``.
:Trace: ``src/hdf5/hdf5_utils.cpp:146-156``, ``src/memory_backend.h:461-466``,
        ``src/mdsplus/mdsplus_backend.cpp:4507-4510``,
        ``src/ascii_backend.cpp:107-157``. Tests
        ``DataEntryModes.CreatePulseFailsWhenAlreadyExists``,
        ``ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists``
        and tripwire
        ``ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently``
        (PR #95).

.. _REQ-DE-005:

REQ-DE-005: FORCE_CREATE_PULSE
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``FORCE_CREATE_PULSE`` (43) shall create an empty data entry,
            discarding all the content of an existing entry at the same
            location.
:Nominal: After ``FORCE_CREATE_PULSE`` on an existing entry, reading any
          IDS that was stored before returns no data.
:Status: Defect for ASCII, wrong result: ASCII only creates the directory
         if it is missing and leaves the existing ``.ids`` files in place,
         so an IDS that is not written again can still be read. HDF5
         deletes the master file and the IDS files; Memory empties the
         entry; MDSplus creates a new tree. IMAS-Python maps its mode
         ``"w"`` to ``FORCE_CREATE_PULSE``.
:Trace: ``src/hdf5/hdf5_utils.cpp:158-160``, ``src/memory_backend.h:483-494``,
        ``src/mdsplus/mdsplus_backend.cpp:4511-4545``,
        ``src/ascii_backend.cpp:128-133``. No test pins the ASCII case.

.. _REQ-DE-006:

REQ-DE-006: Invalid open mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_dataentry_action`` shall refuse any ``mode`` other
            than ``OPEN_PULSE``, ``FORCE_OPEN_PULSE``, ``CREATE_PULSE`` and
            ``FORCE_CREATE_PULSE`` with ``CONTEXT_ERR``, before touching
            the storage and without allocating a handle.
:Errors: ``CLOSE_PULSE``, ``ERASE_PULSE`` or any other integer.
:Status: Defect. The core does not check the mode (the list
         ``alconst::access_pulse_list`` exists but is unused). The backends
         behave differently:

         - HDF5 and MDSplus fail with ``BACKEND_ERR`` ``Mode not yet
           supported``, after the handle is allocated (see
           :ref:`REQ-DE-007`).
         - Memory treats an unknown mode as ``OPEN_PULSE``.
         - ASCII and Flexbuffers accept it; Flexbuffers starts serializing.
:Trace: ``src/al_lowlevel.cpp:856-899``; ``include/al_const.h:118-127``;
        ``src/hdf5/hdf5_backend.cpp:98-99``.

.. _REQ-DE-007:

REQ-DE-007: No handle after a failed open
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When ``al_begin_dataentry_action`` fails, it shall leave
            ``ctx`` unchanged, allocate no handle and leave no backend
            object behind.
:Nominal: For URI errors, ``ctx`` is not written and nothing is created on
          disk.
:Status: Defect, resource leak. When the failure happens after the URI is
         parsed and the backend is created (open mode refused, entry
         missing, version check failed), the handle stays in the table,
         ``ctx`` holds its number and the backend object stays alive until
         the caller calls ``al_end_action`` on a handle it was told is
         invalid. When ``Backend::initBackend`` fails, the
         ``DataEntryContext`` object is leaked.
:Trace: ``src/al_lowlevel.cpp:743-757`` (``beginUriAction``), ``:862-883``.
        Test ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``
        pins the URI case (PR #98).

Backend format version
----------------------

Each backend has a format version ``major.minor``. It is written into a
data entry when the entry is created, and checked when the entry is opened
and before each write.

.. list-table:: Compiled backend format versions
   :header-rows: 1
   :widths: 25 20 55

   * - Backend
     - Version
     - Stored version
   * - HDF5
     - 1.0
     - Attribute ``HDF5_BACKEND_VERSION`` of ``master.h5``.
   * - MDSplus
     - 1.1
     - Nodes ``VERSION:BACK_MAJOR`` and ``VERSION:BACK_MINOR``; 1.0 when
       absent.
   * - Flexbuffers
     - 1.0
     - Not stored; always 1.0.
   * - Memory, ASCII, UDA
     - 0.0
     - Not stored; always 0.0, so the checks never fail.

.. _REQ-DE-010:

REQ-DE-010: Version check when opening
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When an existing data entry is opened with ``OPEN_PULSE`` or
            ``FORCE_OPEN_PULSE``, IMAS-Core shall refuse it if its stored
            major version differs from the compiled major version, or if its
            stored minor version is greater than the compiled minor version.
:Nominal: A file of the same version opens. An MDSplus entry of version
          1.0 opens with the 1.1 backend.
:Errors: ``LOWLEVEL_ERR`` with the message ``Compatibility between opened
         file version <s.major>.<s.minor> and backend <BACKEND_NAME> version
         <major>.<minor> can't be ensured. ABORT.``. For HDF5, an unknown
         stored version is refused earlier with ``BACKEND_ERR`` ``No backend
         reader with version: <v>`` (or ``writer``/``events handler``):
         HDF5 accepts exactly the versions it has readers for, currently
         ``1.0``. An HDF5 file without the attribute fails with
         ``BACKEND_ERR`` ``Not a IMAS HDF5 pulse file. Unable to find
         attribute: ...``.
:Status: Implemented. The handle is still allocated on failure (see
         :ref:`REQ-DE-007`).
:Trace: ``src/al_lowlevel.cpp:868-883``;
        ``src/hdf5/hdf5_backend_factory.cpp:15-48``;
        ``src/hdf5/hdf5_utils.cpp:117``. Tests
        ``Hdf5VersionDrift.MatchingStoredVersionOpensCleanly``,
        ``Hdf5VersionDrift.HigherStoredMajorRefusesOpen``,
        ``Hdf5VersionDrift.NewerStoredMinorRefusesOpen`` (``BACKEND_ERR``,
        PR #98); ``MdsplusVersionDrift.MatchingVersionOpensCleanly``,
        ``MdsplusVersionDrift.MismatchedBackendVersionRefusesOpen``
        (``LOWLEVEL_ERR``, PR #96).

.. _REQ-DE-011:

REQ-DE-011: Version check when writing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A write operation (``WRITE_OP`` with any of the
            ``al_begin_*_action`` functions) shall be refused when the
            stored minor version differs from the compiled minor version.
:Nominal: Entries created by the running library can always be written.
:Limits: An entry with an older minor version can be read but not written:
         an MDSplus entry of version 1.0 cannot be written by the 1.1
         backend.
:Errors: ``LOWLEVEL_ERR`` with the message ``Compatibility between opened
         file version ... can't be ensured (minor versions should match when
         writing). ABORT.``.
:Status: Implemented. The check runs after the backend has started the
         action, see :ref:`REQ-OP-021`.
:Trace: ``src/al_lowlevel.cpp:957-967, 1012-1022, 1076-1086``.

.. _REQ-DE-012:

REQ-DE-012: Version written at creation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A data entry created with ``CREATE_PULSE``,
            ``FORCE_CREATE_PULSE`` or ``FORCE_OPEN_PULSE`` shall store the
            compiled backend format version.
:Errors: MDSplus fails with ``BACKEND_ERR`` ``Cannot save Backend version in
         new pulse`` when the version nodes cannot be written.
:Status: Implemented for HDF5 and MDSplus; not applicable to the other
         backends.
:Trace: ``src/hdf5/hdf5_utils.cpp:366-389``;
        ``src/mdsplus/mdsplus_backend.cpp:43-55``.

Closing
-------

.. _REQ-DE-020:

REQ-DE-020: Close a data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_close_pulse(ctx, CLOSE_PULSE)`` shall release the storage
            resources of the data entry (files, trees, connections) and
            keep its data. The handle stays valid; the caller then calls
            ``al_end_action(ctx)`` to release it and the backend object.
:Nominal: After ``al_close_pulse`` and ``al_end_action``, the entry can be
          opened again with ``OPEN_PULSE`` and holds the same data.
:Limits: Memory keeps the data only while at least one handle on the entry
         is alive; see :ref:`REQ-DE-030`.
:Errors: An unknown or ended handle fails with ``LOWLEVEL_ERR``
         ``Cannot find context <n> in store``. A handle that is not a data
         entry handle fails with ``LOWLEVEL_ERR``
         ``Wrong Context type stored``.
:Status: Implemented. IMAS-Python calls ``al_close_pulse`` then
         ``al_end_action``.
:Trace: ``src/al_lowlevel.cpp:908-934``; IMAS-Python
        ``imas/backends/imas_core/db_entry_al.py:155-159``.

.. _REQ-DE-021:

REQ-DE-021: Erase a data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_close_pulse(ctx, ERASE_PULSE)`` shall close the data entry
            and delete it, so that a later ``OPEN_PULSE`` on the same URI
            fails as for an absent entry.
:Status: Defect, wrong result. HDF5, ASCII, Memory and MDSplus ignore the
         mode and only close the entry; it can still be opened. UDA passes
         the mode to the server as ``erase``. IMAS-Python exposes this as
         ``DBEntry.close(erase=True)``.
:Trace: ``src/hdf5/hdf5_backend.cpp:104-115``,
        ``src/ascii_backend.cpp:161-169``, ``src/memory_backend.h:505-508``,
        ``src/mdsplus/mdsplus_backend.cpp:4554-4563``,
        ``src/uda/uda_utilities.hpp:57``. Tests
        ``ErasePulseKnownDefects.DISABLED_{Hdf5,Ascii,Memory}EraseMakesPulseUnopenable``
        and tripwires
        ``ErasePulseKnownDefects.{Hdf5,Ascii,Memory}EraseCurrentlyLeavesPulseOpenable``
        (PR #95).

.. _REQ-DE-022:

REQ-DE-022: Invalid close mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_close_pulse`` shall refuse a mode other than
            ``CLOSE_PULSE`` and ``ERASE_PULSE`` with ``CONTEXT_ERR``.
:Status: Defect. The mode is not checked; every backend except UDA ignores
         it.
:Trace: ``src/al_lowlevel.cpp:908-934``.

.. _REQ-DE-023:

REQ-DE-023: Use of a closed data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The behavior of ``al_begin_*_action`` or a second
            ``al_close_pulse`` on a data entry that is closed but whose
            handle is not ended is not defined yet.
:Status: Unspecified. It depends on the backend: MDSplus and HDF5 check
         that their file is open in some paths only. A clean definition
         would be a ``LOWLEVEL_ERR`` for any operation after the close.
:Trace: ``src/hdf5/hdf5_backend.cpp:162,169``.

.. _REQ-DE-024:

REQ-DE-024: Ending a data entry without closing it
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The effect of ``al_end_action`` on a data entry handle that was
            not closed with ``al_close_pulse`` is not defined yet.
:Status: Unspecified. ``al_end_action`` deletes the backend object, so the
         result depends on each backend's destructor (whether files are
         flushed and closed). The documented sequence is close, then end.
:Trace: ``src/al_lowlevel.cpp:1640-1675``.

Several handles on one data entry
---------------------------------

.. _REQ-DE-030:

REQ-DE-030: Same data entry opened twice
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Opening the same data entry twice in one process shall give two
            independent handles, and ending one shall not affect the
            other.
:Nominal: Two handles on one HDF5 entry can both be read and then closed
          and ended in any order.
:Status: Defect for Memory, crash. Both handles share one in-memory entry,
         but the Memory backend destructor deletes the shared entry when
         either handle is ended, whatever its reference count. Using or
         ending the other handle is then a use after free. IMAS-Python's
         tests avoid closing the second entry for this reason ("MEM backend
         already cleaned up, prevent SEGFAULT"). A consequence is that the
         data of a Memory entry is lost when the first of its handles is
         ended. Unspecified for the other backends, in particular for
         concurrent writes.
:Trace: ``src/memory_backend.h:414-430`` (``~MemoryBackend``),
        ``:461-467``. IMAS-Python ``imas/test/test_minimal_io.py:20``,
        ``imas/test/test_nbc_change.py:91``.
