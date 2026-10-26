Known defects
=============

This register lists the cases where the baseline does not meet a
requirement. Each entry links to the requirement, which states the intended
behavior and gives the details. Entries are grouped by their effect on the
user:

- **Crash:** the process aborts, crashes, hangs or corrupts memory.
- **Data loss:** stored data is lost or silently not stored.
- **Wrong result:** a call returns wrong data, a wrong status, or succeeds
  when it should fail.
- **Resource leak:** a handle or object is not released.
- **Cosmetic:** messages, console output or documentation.

*Evidence* names the contract test that pins the defect (pull requests #95
to #99; a ``DISABLED_`` test asserts the intended behavior and its tripwire
the current one), the issue or pull request that fixes it, or the code
location when no test exists yet. A defect without a test was found by
reading the code and is not reproduced.

Crash
-----

.. list-table::
   :header-rows: 1
   :widths: 18 52 30

   * - Requirement
     - Defect
     - Evidence
   * - :ref:`REQ-DATA-001`, :ref:`REQ-BE-013`
     - HDF5 crashes when writing or reading a ``CHAR_DATA`` scalar.
     - ``RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips``
   * - :ref:`REQ-DATA-001`, :ref:`REQ-BE-082`
     - ASCII aborts on ``INTEGER_DATA`` at rank 7 and corrupts
       ``DOUBLE_DATA`` and ``COMPLEX_DATA`` at rank 7.
     - ``RoundTripKnownDefects``
   * - :ref:`REQ-TIME-020`, :ref:`REQ-BE-035`
     - MDSplus segfaults on a scalar write inside a slice operation.
     - ``MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds``
   * - :ref:`REQ-CTX-003`
     - ``al_end_action`` on a handle that was already ended dereferences a
       null pointer.
     - ``src/al_lowlevel.cpp:1640-1675``
   * - :ref:`REQ-CTX-011`, :ref:`REQ-AOS-011`
     - ``al_iterate_over_arraystruct`` casts any handle to an AoS context
       without a check, and writes into the wrong object.
     - ``src/al_lowlevel.cpp:1329``
   * - :ref:`REQ-AOS-002`
     - A data entry handle given as the parent of an AoS gives an AoS
       context without an operation context.
     - ``src/al_lowlevel.cpp:589-609``
   * - :ref:`REQ-URI-025`
     - The C legacy URI builder copies the options into a 1024-byte buffer
       with ``strcpy``, and dereferences null string arguments.
     - ``src/al_lowlevel.cpp:1389-1395``
   * - :ref:`REQ-OP-031`
     - A time range operation with a null ``dtime_shape`` dereferences it.
     - ``src/al_lowlevel.cpp:1063, 1509``
   * - :ref:`REQ-TIME-030`
     - The time range search uses an unsigned index that underflows and
       reads out of bounds when no sample is at or before ``tmax``.
     - ``src/data_interpolation.cpp:39``
   * - :ref:`REQ-TIME-031`
     - Resampling with ``dt <= 0`` is not checked and gives an infinite or
       negative sample count.
     - ``src/data_interpolation.cpp:75``
   * - :ref:`REQ-TIME-033`
     - Resampling copies ``sizeof(double)`` bytes per element for integer
       and character data (heap overflow), and linear resampling reads one
       sample past the end.
     - ``src/data_interpolation.cpp:552``
   * - :ref:`REQ-TIME-013`
     - Linear interpolation of ``CHAR_DATA`` never assigns its result, which
       breaks resampled time range reads.
     - ``src/data_interpolation.cpp:285-288``
   * - :ref:`REQ-DE-030`, :ref:`REQ-BE-071`
     - The Memory backend frees a shared entry whatever its reference
       count, so two handles on the same path lead to a use after free or
       a double free.
     - ``src/memory_backend.h:414-430``; IMAS-Python test workarounds
   * - :ref:`REQ-NFR-002`
     - The Memory backend unlock function locks its mutex again, so a
       second thread deadlocks.
     - ``src/memory_backend.h:358-361``
   * - :ref:`REQ-PLG-002`
     - A plugin library that ``dlopen`` cannot load is caught only by an
       ``assert``, which release builds remove.
     - ``PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure``
   * - :ref:`REQ-PLG-020`
     - ``al_setvalue_parameter_plugin`` and its scalar variants crash on a
       plugin name that is not registered.
     - ``KnownDefects.DISABLED_SetValue{Generic,IntScalar,DoubleScalar}UnregisteredPluginReturnsError``
   * - :ref:`REQ-PY-040`
     - Reading ``CHAR_DATA`` with rank 3 or more overflows a two-element
       shape buffer.
     - ``python/imas_core/_al_lowlevel.pyx:925``
   * - :ref:`REQ-PY-031`
     - ``al_write_slice_data`` always raises ``AttributeError``.
     - ``python/imas_core/_al_lowlevel.pyx:662-690``
   * - :ref:`REQ-ERR-007`
     - ``al_context_info``, ``al_get_backendID`` and
       ``al_build_uri_from_legacy_parameters`` catch only one exception
       type, so other C++ exceptions can cross the C interface.
     - ``src/al_lowlevel.cpp:801-854``

Data loss
---------

.. list-table::
   :header-rows: 1
   :widths: 18 52 30

   * - Requirement
     - Defect
     - Evidence
   * - :ref:`REQ-DATA-030`
     - HDF5 ignores the path given to ``al_delete_data`` and deletes the
       whole IDS occurrence.
     - Issue #63, fixed by PR #101 (ADR 0001)
   * - :ref:`REQ-TIME-020`
     - HDF5 keeps only the last of several slices appended with
       ``UNDEFINED_TIME``.
     - ``Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist``
   * - :ref:`REQ-DE-004`, :ref:`REQ-BE-080`
     - ASCII ``CREATE_PULSE`` succeeds on an existing entry, which is then
       overwritten.
     - ``ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists``
   * - :ref:`REQ-BE-054`
     - A UDA remote write returns status 0 but nothing is stored.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-PY-030`
     - The Python binding casts ``int64`` arrays to 32 bits, and measures
       string list items in characters instead of UTF-8 bytes, which
       truncates non-ASCII strings.
     - ``python/imas_core/_al_lowlevel.pyx:44-57, 114-151``

Wrong result
------------

.. list-table::
   :header-rows: 1
   :widths: 18 52 30

   * - Requirement
     - Defect
     - Evidence
   * - :ref:`REQ-DATA-012`
     - HDF5 fails with ``Unable to create HDF5 dataset`` on a second
       whole-IDS write to the same open data entry.
     - Issue #60, fixed by PR #100
   * - :ref:`REQ-DATA-024`
     - HDF5 returns ``EMPTY_*`` with status 0 for an absolute field path
       read inside an AoS context.
     - Issue #65, fixed by PR #103 (ADR 0002)
   * - :ref:`REQ-DATA-030`
     - Deleting is incomplete outside HDF5: Memory ignores a whole-IDS
       delete, ASCII and Flexbuffers ignore every delete, MDSplus fails
       with ``%TREE-W-NNF`` on structures and whole IDSs.
     - ``DeleteKnownDefects.*``, ``test_mdsplus_delete.cpp``
   * - :ref:`REQ-DE-005`
     - ASCII ``FORCE_CREATE_PULSE`` leaves the old IDS files readable.
     - ``src/ascii_backend.cpp:128-133``
   * - :ref:`REQ-DE-006`
     - The open mode is not checked; each backend handles an invalid mode
       differently.
     - ``src/al_lowlevel.cpp:856-899``
   * - :ref:`REQ-DE-021`
     - ``ERASE_PULSE`` behaves like ``CLOSE_PULSE`` on HDF5, ASCII, Memory
       and MDSplus.
     - ``ErasePulseKnownDefects.*``
   * - :ref:`REQ-URI-003`
     - A URI with a host and no ``/`` before the query takes part of the
       query as the host.
     - ``include/uri_parser.h:269-278``
   * - :ref:`REQ-URI-021`
     - Legacy mode drops every query option except ``path``.
     - ``src/al_context.cpp:181-183``
   * - :ref:`REQ-URI-023`
     - The legacy URI builder keeps only the first character of the
       version.
     - ``src/al_context.cpp:234, 252``
   * - :ref:`REQ-URI-024`
     - The legacy URI builder refuses ``FLEXBUFFERS_BACKEND``.
     - ``RoundTripKnownDefects``
   * - :ref:`REQ-CTX-011`, :ref:`REQ-QRY-011`
     - ``al_get_backendID`` accepts any context and returns an undefined
       value with status 0.
     - ``GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError``
   * - :ref:`REQ-OP-031`
     - The C API checks neither the interpolation mode nor ``tmin <= tmax``
       of a time range.
     - ``src/al_context.cpp:353-388``
   * - :ref:`REQ-OP-032`, :ref:`REQ-TIME-034`
     - A time range context leaves its time and interpolation mode
       uninitialized; UDA time range reads fail with "unknown interp
       mode".
     - UDA contract tests (PR #97)
   * - :ref:`REQ-TIME-013`
     - ``COMPLEX_DATA`` is not interpolated; the result is left unset.
     - ``src/data_interpolation.cpp:179-290``
   * - :ref:`REQ-AOS-031`, :ref:`REQ-BE-083`
     - ASCII reports an AoS size of 0 unless the AoS is read right after the
       preceding field.
     - ``AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize``
   * - :ref:`REQ-AOS-031`, :ref:`REQ-BE-054`
     - UDA returns the empty value for a dynamic leaf inside an AoS.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-BE-051`
     - UDA silently loads a Data Dictionary of the wrong version.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-BE-052`
     - UDA ``cache_mode=struct`` returns the empty value for top-level
       fields.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-BE-055`
     - The UDA client version check never fires.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-BE-057`
     - Closing a UDA session leaves a handle open on the server.
     - UDA contract tests (PR #97)
   * - :ref:`REQ-ERR-003`, :ref:`REQ-ERR-004`, :ref:`REQ-BE-058`,
       :ref:`REQ-QRY-002`
     - URI and context errors, and some backend errors, are reported as
       ``UNKNOWN_ERR`` instead of ``CONTEXT_ERR`` or ``BACKEND_ERR``.
     - ``BackendFactory.*`` (PR #98)
   * - :ref:`REQ-ERR-011`
     - The status message is left uninitialized on success.
     - ``src/al_lowlevel.cpp``
   * - :ref:`REQ-PLG-003`
     - Unregistering a plugin that was never bound leaves it registered.
     - ``PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt``
   * - :ref:`REQ-PLG-011`
     - ``al_unbind_plugin`` does not normalize the path, so a path given in
       the form used to bind does not unbind.
     - ``src/al_lowlevel.cpp:297-316``
   * - :ref:`REQ-PLG-012`
     - The ``ids:*/*`` wildcard never matches, and a plugin bound to
       occurrence 10 is also called for occurrence 1.
     - ``src/al_lowlevel.cpp:147-148, 181-182``
   * - :ref:`REQ-PY-011`, :ref:`REQ-PY-020`
     - The Python exception switch covers only some functions, and return
       values have different shapes on success and failure.
     - ``python/imas_core/_al_lowlevel.pyx:158-518``

Resource leak
-------------

.. list-table::
   :header-rows: 1
   :widths: 18 52 30

   * - Requirement
     - Defect
     - Evidence
   * - :ref:`REQ-DE-007`
     - A failed open leaves the handle allocated and ``ctx`` set.
     - ``src/al_lowlevel.cpp:743-757, 862-883``
   * - :ref:`REQ-CTX-020`
     - ``al_end_action`` removes the handle before ending the backend
       action, so an exception leaks the context.
     - ``src/al_lowlevel.cpp:1648``
   * - :ref:`REQ-OP-021`
     - An operation context leaks when the backend refuses to begin the
       action or the write version check fails.
     - ``src/al_lowlevel.cpp:950-968``
   * - :ref:`REQ-AOS-003`, :ref:`REQ-AOS-004`
     - An AoS of size 0 or a negative size leaves its handle slot
       allocated.
     - ``src/al_lowlevel.cpp:1283-1298, 1601-1618``

Cosmetic
--------

.. list-table::
   :header-rows: 1
   :widths: 18 52 30

   * - Requirement
     - Defect
     - Evidence
   * - :ref:`REQ-URI-006`
     - The URI fragment is set to the whole query when there is no ``#``.
     - ``include/uri_parser.h:338``
   * - :ref:`REQ-DE-022`
     - The close mode is not checked.
     - ``src/al_lowlevel.cpp:908``
   * - :ref:`REQ-ERR-013`, :ref:`REQ-NFR-022`, :ref:`REQ-DATA-022`
     - The library prints errors and debug text on standard output and
       standard error.
     - ``src/al_lowlevel.cpp:1231``
   * - :ref:`REQ-PY-002`
     - ``TIMERANGE_OP`` and ``FLEXBUFFERS_BACKEND`` are not exported to
       Python.
     - ``python/imas_core/al_defs.pyx``
   * - :ref:`REQ-PY-012`
     - Some Python exceptions carry the message as ``bytes``.
     - ``python/imas_core/_al_lowlevel.pyx:318``
   * - :ref:`REQ-PY-052`
     - The Cython declaration of the status message has 10 characters
       instead of 256.
     - ``python/imas_core/al_lowlevel_interface.pxd:5``
   * - :ref:`REQ-NFR-020`
     - Several environment variables, among them ``IMAS_AL_PLUGINS``, are
       not documented.
     - :doc:`../user_guide/configuration`

Documentation inconsistencies
-----------------------------

These do not affect the library, but the existing documentation
contradicts this specification:

- :doc:`../developers/development` says arrays have 1 to 6 dimensions;
  IMAS-Core handles up to ``MAXDIM`` (7), see :ref:`REQ-DATA-001`.
- ``README.md`` checks the installation with ``import imas``; the package
  installed by ``pip install imas-core`` is ``imas_core``.
- The backend table in ``README.md`` says MDSplus is remote and not
  file-based, and Flexbuffers is file-based. Through IMAS-Core, MDSplus
  stores tree files under the ``path`` option (remote access goes through
  UDA), and
  Flexbuffers serializes to memory, see :doc:`backends`.
