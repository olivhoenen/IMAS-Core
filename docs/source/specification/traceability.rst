Traceability
============

This page maps each requirement to its status, to the tests that check it
and to the pull requests and IMAS-Python code it refers to. It is generated
from the *Status* and *Trace* fields of the requirements; the code
references are in the requirements themselves.

Test names of the form ``Suite.Case`` are GoogleTest cases of the contract
test suite (``tests/contract/``, pull requests #95 to #103). Files named
``test_*.py`` are in ``python/tests/`` unless the trace says IMAS-Python.

Summary
-------

.. list-table::
   :header-rows: 1
   :widths: 40 20

   * - Status
     - Requirements
   * - Implemented
     - 85
   * - Defect
     - 71
   * - Divergence
     - 10
   * - Unspecified
     - 24
   * - Total
     - 176

A requirement can have several statuses, for example *Implemented* on
some backends and *Defect* on others.

Matrix
------

.. list-table::
   :header-rows: 1
   :widths: 14 30 16 30 10

   * - Requirement
     - Title
     - Status
     - Tests
     - Pull requests
   * - :ref:`REQ-AOS-001`
     - Begin an array of structures action
     - Implemented
     - ``Backends/AosMatrix.{TopLevelWriteIterateRead,NestedWriteIterateRead}``
     - --
   * - :ref:`REQ-AOS-002`
     - Context type check
     - Defect
     - --
     - --
   * - :ref:`REQ-AOS-003`
     - Empty array of structures
     - Defect
     - --
     - --
   * - :ref:`REQ-AOS-004`
     - Negative size
     - Defect
     - --
     - --
   * - :ref:`REQ-AOS-005`
     - Plugins and AoS contexts
     - Implemented
     - --
     - --
   * - :ref:`REQ-AOS-010`
     - Current element and iteration
     - Implemented, Unspecified
     - ``test_lazy_loading.py``
     - --
   * - :ref:`REQ-AOS-011`
     - Iterating a handle that is not an AoS context
     - Defect
     - --
     - --
   * - :ref:`REQ-AOS-020`
     - Ending an AoS context
     - Implemented
     - --
     - --
   * - :ref:`REQ-AOS-030`
     - Timed AoS in slice and time range reads
     - Implemented
     - --
     - #96
   * - :ref:`REQ-AOS-031`
     - Backend support for arrays of structures
     - Defect, Divergence
     - ``AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize``, ``Backends/AosMatrix.*/Flexbuffers``
     - #97
   * - :ref:`REQ-BE-001`
     - Backends not built in are reported
     - Implemented
     - --
     - --
   * - :ref:`REQ-BE-002`
     - Capability flags match the behavior
     - Divergence
     - --
     - --
   * - :ref:`REQ-BE-003`
     - Backend version stamps
     - Implemented
     - ``Hdf5VersionDrift.*``, ``MdsplusVersionDrift.*``
     - --
   * - :ref:`REQ-BE-004`
     - Handling of backend query keys
     - Unspecified
     - --
     - --
   * - :ref:`REQ-BE-010`
     - HDF5 data entry layout and open modes
     - Implemented
     - ``Backends/DataEntryModes.*``
     - --
   * - :ref:`REQ-BE-011`
     - HDF5 format version
     - Implemented
     - ``Hdf5VersionDrift.*``
     - --
   * - :ref:`REQ-BE-012`
     - HDF5 read-only files and concurrent writers
     - Implemented
     - ``test_hdf5_permissions.py``
     - --
   * - :ref:`REQ-BE-013`
     - HDF5 data types and ranks
     - Defect
     - ``Backends/RoundTripMatrix.ReadEqualsWrite/HDF5_*``, ``RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips``, ``RoundTripKnownDefectsDeath.Hdf5CharScalarCurrentlyCrashes``
     - --
   * - :ref:`REQ-BE-014`
     - HDF5 interpolation warning
     - Implemented
     - --
     - --
   * - :ref:`REQ-BE-030`
     - MDSplus data entry creation and open modes
     - Implemented
     - ``Backends/DataEntryModes.*/Mdsplus``
     - --
   * - :ref:`REQ-BE-031`
     - MDSplus format version
     - Unspecified
     - --
     - --
   * - :ref:`REQ-BE-032`
     - MDSplus accepts only Data Dictionary paths
     - Divergence
     - ``Backends/AosMatrix.*/Mdsplus``, ``Mdsplus/MdsplusRealPathMatrix.RealPathRoundTrip/*``
     - --
   * - :ref:`REQ-BE-033`
     - MDSplus time-dependent strings
     - Divergence
     - --
     - --
   * - :ref:`REQ-BE-034`
     - MDSplus slices
     - Implemented
     - ``MdsplusSegmentBackedWrites.AppendedElementsAllPersist``, ``MdsplusStructAwareSliceInterpolation.*``, ``MdsplusTimebaseCache.SurvivesInterveningWritesWithoutStaleness``, ``MdsplusTimerangeResampling.RefusedWithNoUniqueSurface``
     - --
   * - :ref:`REQ-BE-035`
     - MDSplus scalar write in a slice action
     - Defect
     - ``MdsplusSliceWriteKnownDefects.DISABLED_ScalarWriteWithinSliceActionSucceeds``, ``MdsplusSliceWriteKnownDefectsDeath.ScalarWriteWithinSliceActionCurrentlyCrashes``
     - --
   * - :ref:`REQ-BE-050`
     - UDA server and plugin selection
     - Implemented
     - ``UdaUniqueSurfaceTest.*``
     - --
   * - :ref:`REQ-BE-051`
     - UDA Data Dictionary loading
     - Defect
     - ``UdaUniqueSurfaceTest.DISABLED_DdWrongVersionIsRejected``
     - --
   * - :ref:`REQ-BE-052`
     - UDA cache modes
     - Defect
     - ``UdaUniqueSurfaceTest.DISABLED_CacheModeStructPreservesTopLevelField``
     - #97
   * - :ref:`REQ-BE-053`
     - UDA fetch mode
     - Divergence
     - --
     - --
   * - :ref:`REQ-BE-054`
     - UDA remote access is read-only
     - Defect
     - ``UdaAosKnownDefects.DISABLED_DynamicLeafInsideAosRoundTrips``, ``UdaUniqueSurfaceTest.DISABLED_RemoteWriteFailsWithDispatchError``, ``UdaUniqueSurfaceTest.RemoteDeleteFailsWithUnknownFunctionRequested``
     - --
   * - :ref:`REQ-BE-055`
     - UDA version check
     - Defect
     - ``UdaUniqueSurfaceTest.DISABLED_OpenRefusesMismatchedStoredBackendVersion``
     - --
   * - :ref:`REQ-BE-056`
     - UDA capability negotiation
     - Implemented
     - ``UdaUniqueSurfaceTest.TimeRangeCapabilityGrantedByReferenceServerVersion180``
     - --
   * - :ref:`REQ-BE-057`
     - UDA session release
     - Defect
     - ``UdaUniqueSurfaceTest.DISABLED_ClosingUdaSessionReleasesEveryServerSideHandle``
     - --
   * - :ref:`REQ-BE-058`
     - UDA error classes
     - Defect
     - --
     - --
   * - :ref:`REQ-BE-070`
     - Memory store and open modes
     - Implemented
     - ``Backends/DataEntryModes.*/Memory``
     - --
   * - :ref:`REQ-BE-071`
     - Memory store lifetime
     - Defect
     - ``test_minimal_io.py``, ``test_nbc_change.py``
     - --
   * - :ref:`REQ-BE-072`
     - Memory slices
     - Divergence
     - ``CapabilityMatrix.SliceReadPositiveOrRefused/Memory``, ``CapabilityMatrix.SliceWriteBeginPositiveOrRefused/Memory``
     - --
   * - :ref:`REQ-BE-073`
     - Memory queries
     - Unspecified
     - ``test_list_occurrences.py``
     - --
   * - :ref:`REQ-BE-080`
     - ASCII data entry layout and open modes
     - Defect
     - ``ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently``, ``ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists``
     - --
   * - :ref:`REQ-BE-081`
     - ASCII operations
     - Divergence
     - ``CapabilityMatrix.*/ASCII``
     - --
   * - :ref:`REQ-BE-082`
     - ASCII data types and ranks
     - Defect
     - ``RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips``
     - --
   * - :ref:`REQ-BE-083`
     - ASCII arrays of structures
     - Defect
     - ``AosKnownDefects.AsciiAosReadCurrentlyReportsZero``, ``AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize``
     - --
   * - :ref:`REQ-BE-090`
     - Flexbuffers serialization protocol
     - Implemented
     - --
     - --
   * - :ref:`REQ-BE-091`
     - Flexbuffers restrictions
     - Implemented
     - ``Backends/RoundTripMatrix.ReadEqualsWrite/Flexbuffers_*``
     - --
   * - :ref:`REQ-BE-100`
     - NoBackend
     - Unspecified
     - --
     - --
   * - :ref:`REQ-CTX-001`
     - Handle values
     - Implemented
     - --
     - --
   * - :ref:`REQ-CTX-002`
     - Handle 0
     - Implemented
     - ``ContextInfo.NullContextReturnsLiteralString``
     - #95
   * - :ref:`REQ-CTX-003`
     - Unknown or ended handle
     - Defect
     - ``UdaBreadthTest.AosTraversalAdvancesAcrossDistinctStaticElements``
     - #97
   * - :ref:`REQ-CTX-004`
     - Handle reuse
     - Implemented
     - --
     - --
   * - :ref:`REQ-CTX-010`
     - Context tree
     - Implemented
     - --
     - --
   * - :ref:`REQ-CTX-011`
     - Context type checks
     - Defect
     - ``GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError``, ``GetBackendIdKnownDefects.WrongContextTypeCurrentlySucceedsViaUnsafeCast``
     - #95
   * - :ref:`REQ-CTX-012`
     - Ending order
     - Unspecified
     - --
     - --
   * - :ref:`REQ-CTX-013`
     - Several operations on one data entry
     - Unspecified
     - --
     - --
   * - :ref:`REQ-CTX-020`
     - al_end_action
     - Defect
     - --
     - --
   * - :ref:`REQ-DATA-001`
     - Data types and ranks
     - Defect, Unspecified
     - ``Backends/RoundTripMatrix.ReadEqualsWrite/*``, ``RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips``, ``RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips``
     - #95
   * - :ref:`REQ-DATA-002`
     - Buffer layout
     - Implemented
     - --
     - --
   * - :ref:`REQ-DATA-003`
     - Strings
     - Defect
     - --
     - --
   * - :ref:`REQ-DATA-004`
     - Field and timebase paths
     - Defect
     - --
     - --
   * - :ref:`REQ-DATA-010`
     - Write and read back
     - Implemented, Unspecified
     - ``Backends/RoundTripMatrix.*``, ``RoundTrip.*``
     - #95
   * - :ref:`REQ-DATA-011`
     - Empty values are not written
     - Implemented
     - --
     - --
   * - :ref:`REQ-DATA-012`
     - Rewriting an IDS occurrence
     - Defect
     - ``EquilibriumSeedMatrix.RepeatedWholeIdsWriteOnSameOpenPulseSucceeds``
     - #100
   * - :ref:`REQ-DATA-020`
     - Read result and buffer ownership
     - Implemented
     - --
     - --
   * - :ref:`REQ-DATA-021`
     - Missing data
     - Divergence
     - --
     - --
   * - :ref:`REQ-DATA-022`
     - Rank mismatch
     - Implemented
     - --
     - --
   * - :ref:`REQ-DATA-023`
     - Type mismatch
     - Unspecified
     - --
     - --
   * - :ref:`REQ-DATA-024`
     - Absolute field paths in reads
     - Defect
     - ``Hdf5AbsoluteFieldPaths.*``
     - #103
   * - :ref:`REQ-DATA-030`
     - Delete path semantics
     - Defect
     - ``Backends/DeleteMatrix.*``, ``DeleteKnownDefects.*``, ``Hdf5Delete.*``, ``test_mdsplus_delete.cpp``
     - #101
   * - :ref:`REQ-DATA-031`
     - Delete access mode
     - Unspecified
     - --
     - --
   * - :ref:`REQ-DE-001`
     - Open a data entry
     - Implemented
     - --
     - --
   * - :ref:`REQ-DE-002`
     - OPEN_PULSE
     - Implemented, Divergence
     - ``DataEntryModes.OpenPulseFailsWhenAbsent``, ``DataEntryModes.OpenPulseSucceedsOncePresent``
     - #95, #96
   * - :ref:`REQ-DE-003`
     - FORCE_OPEN_PULSE
     - Implemented
     - ``DataEntryModes.ForceOpenPulseCreatesWhenAbsent``
     - #95
   * - :ref:`REQ-DE-004`
     - CREATE_PULSE
     - Implemented, Defect
     - ``DataEntryModes.CreatePulseFailsWhenAlreadyExists``, ``ModeKnownDefects.AsciiCreatePulseCurrentlyOverwritesSilently``, ``ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists``
     - #95
   * - :ref:`REQ-DE-005`
     - FORCE_CREATE_PULSE
     - Defect
     - --
     - --
   * - :ref:`REQ-DE-006`
     - Invalid open mode
     - Defect
     - --
     - --
   * - :ref:`REQ-DE-007`
     - No handle after a failed open
     - Defect
     - ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``
     - #98
   * - :ref:`REQ-DE-010`
     - Version check when opening
     - Implemented
     - ``Hdf5VersionDrift.HigherStoredMajorRefusesOpen``, ``Hdf5VersionDrift.MatchingStoredVersionOpensCleanly``, ``Hdf5VersionDrift.NewerStoredMinorRefusesOpen``, ``MdsplusVersionDrift.MatchingVersionOpensCleanly``, ``MdsplusVersionDrift.MismatchedBackendVersionRefusesOpen``
     - #96, #98
   * - :ref:`REQ-DE-011`
     - Version check when writing
     - Implemented
     - --
     - --
   * - :ref:`REQ-DE-012`
     - Version written at creation
     - Implemented
     - --
     - --
   * - :ref:`REQ-DE-020`
     - Close a data entry
     - Implemented
     - --
     - --
   * - :ref:`REQ-DE-021`
     - Erase a data entry
     - Defect
     - ``ErasePulseKnownDefects.DISABLED_{Hdf5,Ascii,Memory}EraseMakesPulseUnopenable``, ``ErasePulseKnownDefects.{Hdf5,Ascii,Memory}EraseCurrentlyLeavesPulseOpenable``
     - #95
   * - :ref:`REQ-DE-022`
     - Invalid close mode
     - Defect
     - --
     - --
   * - :ref:`REQ-DE-023`
     - Use of a closed data entry
     - Unspecified
     - --
     - --
   * - :ref:`REQ-DE-024`
     - Ending a data entry without closing it
     - Unspecified
     - --
     - --
   * - :ref:`REQ-DE-030`
     - Same data entry opened twice
     - Defect, Unspecified
     - ``test_minimal_io.py``, ``test_nbc_change.py``
     - --
   * - :ref:`REQ-ERR-001`
     - Status structure
     - Implemented
     - --
     - --
   * - :ref:`REQ-ERR-002`
     - Meaning of the codes
     - Implemented
     - --
     - --
   * - :ref:`REQ-ERR-003`
     - Context errors keep their code
     - Defect
     - ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``
     - #98
   * - :ref:`REQ-ERR-004`
     - Backend errors keep their code
     - Defect
     - --
     - --
   * - :ref:`REQ-ERR-005`
     - Errors of al_build_uri_from_legacy_parameters
     - Implemented
     - ``BackendFactory.UnknownPositiveLegacyBackendIdIsRefused``
     - #98
   * - :ref:`REQ-ERR-006`
     - Plugin errors
     - Implemented
     - --
     - --
   * - :ref:`REQ-ERR-007`
     - No exception crosses the C API
     - Defect
     - --
     - --
   * - :ref:`REQ-ERR-010`
     - Message content
     - Implemented
     - ``test_exception.py``
     - --
   * - :ref:`REQ-ERR-011`
     - Message on success
     - Defect
     - --
     - --
   * - :ref:`REQ-ERR-012`
     - Warnings
     - Implemented
     - --
     - --
   * - :ref:`REQ-ERR-013`
     - No output on the standard streams
     - Defect
     - --
     - --
   * - :ref:`REQ-ERR-020`
     - C++ exception classes
     - Implemented
     - --
     - --
   * - :ref:`REQ-ERR-021`
     - Python exceptions
     - Implemented
     - ``test_exception.py``
     - --
   * - :ref:`REQ-NFR-001`
     - Use from several threads
     - Unspecified
     - --
     - --
   * - :ref:`REQ-NFR-002`
     - Memory backend locking
     - Defect
     - --
     - --
   * - :ref:`REQ-NFR-010`
     - Memory returned to the caller
     - Implemented
     - ``CapabilityMatrix.ListFilledPathsPositiveOrRefused``, ``ContextInfo.*``, ``Occurrences.*``, ``UriOwnership.CallerFreesBuiltUri``
     - --
   * - :ref:`REQ-NFR-020`
     - Environment variables
     - Defect
     - --
     - --
   * - :ref:`REQ-NFR-021`
     - Changes to the process environment
     - Implemented
     - --
     - --
   * - :ref:`REQ-NFR-022`
     - Console output
     - Defect
     - --
     - --
   * - :ref:`REQ-NFR-030`
     - API versioning
     - Implemented
     - --
     - --
   * - :ref:`REQ-NFR-031`
     - Stored data compatibility
     - Implemented
     - ``Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted``
     - --
   * - :ref:`REQ-OP-001`
     - Start an operation
     - Implemented
     - --
     - --
   * - :ref:`REQ-OP-002`
     - Access mode
     - Unspecified
     - --
     - --
   * - :ref:`REQ-OP-003`
     - IDS name and occurrence
     - Implemented
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-OP-004`
     - Plugin notification
     - Implemented
     - --
     - --
   * - :ref:`REQ-OP-010`
     - Global operation
     - Implemented
     - --
     - --
   * - :ref:`REQ-OP-011`
     - Data path of a global read
     - Implemented
     - --
     - --
   * - :ref:`REQ-OP-020`
     - Slice operation arguments
     - Implemented
     - ``CapabilityMatrix.SliceReadPositiveOrRefused``, ``CapabilityMatrix.SliceWriteBeginPositiveOrRefused``, ``Hdf5TimeDependent.SliceInterpolationModes``, ``Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected``
     - #95
   * - :ref:`REQ-OP-021`
     - No leak when an operation cannot start
     - Defect
     - --
     - --
   * - :ref:`REQ-OP-030`
     - Time range operation arguments
     - Implemented
     - ``CapabilityMatrix.TimeRangeReadPositiveOrRefused``, ``Hdf5TimeDependent.TimeRangeReadWithResampling``, ``Hdf5TimeDependent.TimeRangeReadWithoutResampling``
     - #95
   * - :ref:`REQ-OP-031`
     - Time range argument checks
     - Defect
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-OP-032`
     - Complete time range context
     - Defect
     - --
     - #97
   * - :ref:`REQ-PLG-001`
     - Plugin framework switch
     - Implemented
     - --
     - --
   * - :ref:`REQ-PLG-002`
     - Registering a plugin
     - Defect
     - ``PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure``
     - --
   * - :ref:`REQ-PLG-003`
     - Unregistering a plugin
     - Defect
     - ``PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt``
     - --
   * - :ref:`REQ-PLG-004`
     - Querying registration
     - Implemented
     - ``PluginTest.IsPluginRegisteredIsFalseForNeverRegisteredName``
     - --
   * - :ref:`REQ-PLG-010`
     - Binding a plugin to a path
     - Implemented
     - --
     - --
   * - :ref:`REQ-PLG-011`
     - Unbinding a plugin
     - Defect
     - ``PluginTest.UnbindNeverBoundPathIsSilentNoOp``, ``test_plugins.cpp``
     - #95
   * - :ref:`REQ-PLG-012`
     - Plugins replace the backend for bound paths
     - Defect
     - ``PluginTest.BoundPluginInterceptsReadInsteadOfBackend``, ``PluginTest.BoundPluginInterceptsWriteInsteadOfBackend``
     - --
   * - :ref:`REQ-PLG-013`
     - Low-level re-entry for plugins
     - Implemented
     - ``PluginReentry.*``
     - --
   * - :ref:`REQ-PLG-020`
     - Setting plugin parameters
     - Defect
     - ``KnownDefects.DISABLED_SetValue{Generic,IntScalar,DoubleScalar}UnregisteredPluginReturnsError``, ``PluginTest.SetValueGenericAcceptsDimAboveMaxdim``, ``PluginTest.SetValue{Generic,IntScalar,DoubleScalar}OnRegisteredPluginReachesPlugin``
     - --
   * - :ref:`REQ-PLG-021`
     - Storing plugin provenance
     - Implemented
     - ``PluginTest.WritePluginsMetadataStoresBoundPluginProvenance``
     - --
   * - :ref:`REQ-PLG-022`
     - Readback plugins
     - Unspecified
     - ``ReadbackPlugins.BindReadbackPluginsAutoRegistersAndBindsFromStoredMetadata``
     - --
   * - :ref:`REQ-PY-001`
     - Package layout
     - Implemented
     - ``test_imasdef.py``, ``test_import.py``
     - --
   * - :ref:`REQ-PY-002`
     - Constants
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-010`
     - Exception classes
     - Implemented
     - ``test_exception.py``
     - --
   * - :ref:`REQ-PY-011`
     - Exception mode
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-012`
     - Error messages are text
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-020`
     - Return values of the context functions
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-021`
     - Time range arguments
     - Implemented
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-PY-030`
     - Python types accepted by ``al_write_data``
     - Defect
     - ``test_exception.py``
     - --
   * - :ref:`REQ-PY-031`
     - Writing one slice
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-040`
     - Values returned by ``al_read_data``
     - Defect
     - --
     - --
   * - :ref:`REQ-PY-041`
     - Query results
     - Implemented
     - --
     - --
   * - :ref:`REQ-PY-042`
     - Flexbuffers buffer access
     - Implemented
     - --
     - --
   * - :ref:`REQ-PY-050`
     - Array of structures wrappers
     - Implemented
     - --
     - --
   * - :ref:`REQ-PY-051`
     - Plugin wrappers
     - Implemented
     - --
     - --
   * - :ref:`REQ-PY-052`
     - C declarations match the C header
     - Defect
     - --
     - --
   * - :ref:`REQ-QRY-001`
     - Listing IDS occurrences
     - Divergence, Unspecified
     - ``Occurrences.*``, ``test_list_occurrences.py``
     - --
   * - :ref:`REQ-QRY-002`
     - Listing filled paths
     - Implemented, Defect
     - ``Backends/CapabilityMatrix.ListFilledPathsPositiveOrRefused``, ``test_list_filled_paths.py``
     - --
   * - :ref:`REQ-QRY-010`
     - Describing a context
     - Implemented
     - ``ContextInfo.*``
     - --
   * - :ref:`REQ-QRY-011`
     - Backend of a data entry
     - Defect
     - ``GetBackendId.ReturnsTheBackendUsedToOpen``, ``GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError``
     - --
   * - :ref:`REQ-QRY-020`
     - Library version
     - Implemented
     - --
     - --
   * - :ref:`REQ-QRY-021`
     - No Data Dictionary version
     - Implemented
     - ``Backends/VersionSentinel.VersionPutRoundTripsOpaquelyAndIsNotInterpreted``, ``Introspection.GetDDVersionReturnsDeprecatedSentinel``, ``test_imasdef.py``
     - --
   * - :ref:`REQ-QRY-022`
     - Names of constants and error codes
     - Implemented, Unspecified
     - ``Introspection.*``
     - --
   * - :ref:`REQ-TIME-001`
     - Timebases
     - Implemented, Unspecified
     - --
     - --
   * - :ref:`REQ-TIME-010`
     - Slice read result
     - Implemented
     - ``Backends/CapabilityMatrix.SliceReadPositiveOrRefused``, ``Hdf5TimeDependent.SliceInterpolationModes``, ``Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected``
     - --
   * - :ref:`REQ-TIME-011`
     - CLOSEST_INTERP
     - Implemented
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-TIME-012`
     - PREVIOUS_INTERP
     - Implemented, Unspecified
     - --
     - --
   * - :ref:`REQ-TIME-013`
     - LINEAR_INTERP
     - Defect
     - --
     - --
   * - :ref:`REQ-TIME-020`
     - Appending a slice
     - Defect
     - ``Backends/CapabilityMatrix.SliceWriteBeginPositiveOrRefused``, ``Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist``
     - --
   * - :ref:`REQ-TIME-030`
     - Time range without resampling
     - Defect
     - ``Hdf5TimeDependent.TimeRangeReadWithoutResampling``, ``test_get_sample.py``
     - --
   * - :ref:`REQ-TIME-031`
     - Resampling on a uniform time step
     - Defect
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-TIME-032`
     - Resampling on an explicit time vector
     - Implemented, Unspecified
     - ``test_get_sample.py``
     - --
   * - :ref:`REQ-TIME-033`
     - Data types in resampling
     - Defect
     - --
     - --
   * - :ref:`REQ-TIME-034`
     - Time range support
     - Implemented, Defect
     - ``Backends/CapabilityMatrix.TimeRangeReadPositiveOrRefused``
     - --
   * - :ref:`REQ-URI-001`
     - URI structure
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-002`
     - URI scheme
     - Unspecified
     - --
     - --
   * - :ref:`REQ-URI-003`
     - End of the authority
     - Defect
     - --
     - --
   * - :ref:`REQ-URI-004`
     - Query syntax
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-005`
     - Repeated query keys
     - Unspecified
     - --
     - --
   * - :ref:`REQ-URI-006`
     - Fragment
     - Defect
     - --
     - --
   * - :ref:`REQ-URI-010`
     - Backend selected from the URI path
     - Implemented
     - ``BackendFactory.EmptyBackendNameInUriReturnsCleanError``, ``BackendFactory.UnknownBackendNameInUriReturnsCleanError``, ``BackendFactory.UnknownBackendNameIsRefusedForEveryOpenMode``
     - #98
   * - :ref:`REQ-URI-011`
     - Backend not built
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-012`
     - Location given by path or mapping
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-013`
     - Local redirection of UDA URIs
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-020`
     - Legacy URI resolution
     - Implemented
     - --
     - --
   * - :ref:`REQ-URI-021`
     - Other keys kept in legacy mode
     - Defect
     - --
     - --
   * - :ref:`REQ-URI-022`
     - Building a URI from legacy parameters
     - Implemented
     - ``BackendFactory.UnknownPositiveLegacyBackendIdIsRefused``, ``UriOwnership.CallerFreesBuiltUri``
     - #95, #98
   * - :ref:`REQ-URI-023`
     - Major version in built URIs
     - Defect
     - --
     - --
   * - :ref:`REQ-URI-024`
     - Flexbuffers in built URIs
     - Defect
     - ``RoundTripKnownDefects.BuildUriFlexbuffersCurrentlyFails``, ``RoundTripKnownDefects.DISABLED_BuildUriSupportsFlexbuffers``
     - #95
   * - :ref:`REQ-URI-025`
     - Argument checks in the C builder
     - Defect
     - --
     - --

Requirements without a named test
---------------------------------

These requirements are not checked by any test named in their trace. Some
are checked indirectly by broader tests; the others need a test.

- :ref:`REQ-AOS-002` (Defect)
- :ref:`REQ-AOS-003` (Defect)
- :ref:`REQ-AOS-004` (Defect)
- :ref:`REQ-AOS-005` (Implemented)
- :ref:`REQ-AOS-011` (Defect)
- :ref:`REQ-AOS-020` (Implemented)
- :ref:`REQ-AOS-030` (Implemented)
- :ref:`REQ-BE-001` (Implemented)
- :ref:`REQ-BE-002` (Divergence)
- :ref:`REQ-BE-004` (Unspecified)
- :ref:`REQ-BE-014` (Implemented)
- :ref:`REQ-BE-031` (Unspecified)
- :ref:`REQ-BE-033` (Divergence)
- :ref:`REQ-BE-053` (Divergence)
- :ref:`REQ-BE-058` (Defect)
- :ref:`REQ-BE-090` (Implemented)
- :ref:`REQ-BE-100` (Unspecified)
- :ref:`REQ-CTX-001` (Implemented)
- :ref:`REQ-CTX-004` (Implemented)
- :ref:`REQ-CTX-010` (Implemented)
- :ref:`REQ-CTX-012` (Unspecified)
- :ref:`REQ-CTX-013` (Unspecified)
- :ref:`REQ-CTX-020` (Defect)
- :ref:`REQ-DATA-002` (Implemented)
- :ref:`REQ-DATA-003` (Defect)
- :ref:`REQ-DATA-004` (Defect)
- :ref:`REQ-DATA-011` (Implemented)
- :ref:`REQ-DATA-020` (Implemented)
- :ref:`REQ-DATA-021` (Divergence)
- :ref:`REQ-DATA-022` (Implemented)
- :ref:`REQ-DATA-023` (Unspecified)
- :ref:`REQ-DATA-031` (Unspecified)
- :ref:`REQ-DE-001` (Implemented)
- :ref:`REQ-DE-005` (Defect)
- :ref:`REQ-DE-006` (Defect)
- :ref:`REQ-DE-011` (Implemented)
- :ref:`REQ-DE-012` (Implemented)
- :ref:`REQ-DE-020` (Implemented)
- :ref:`REQ-DE-022` (Defect)
- :ref:`REQ-DE-023` (Unspecified)
- :ref:`REQ-DE-024` (Unspecified)
- :ref:`REQ-ERR-001` (Implemented)
- :ref:`REQ-ERR-002` (Implemented)
- :ref:`REQ-ERR-004` (Defect)
- :ref:`REQ-ERR-006` (Implemented)
- :ref:`REQ-ERR-007` (Defect)
- :ref:`REQ-ERR-011` (Defect)
- :ref:`REQ-ERR-012` (Implemented)
- :ref:`REQ-ERR-013` (Defect)
- :ref:`REQ-ERR-020` (Implemented)
- :ref:`REQ-NFR-001` (Unspecified)
- :ref:`REQ-NFR-002` (Defect)
- :ref:`REQ-NFR-020` (Defect)
- :ref:`REQ-NFR-021` (Implemented)
- :ref:`REQ-NFR-022` (Defect)
- :ref:`REQ-NFR-030` (Implemented)
- :ref:`REQ-OP-001` (Implemented)
- :ref:`REQ-OP-002` (Unspecified)
- :ref:`REQ-OP-004` (Implemented)
- :ref:`REQ-OP-010` (Implemented)
- :ref:`REQ-OP-011` (Implemented)
- :ref:`REQ-OP-021` (Defect)
- :ref:`REQ-OP-032` (Defect)
- :ref:`REQ-PLG-001` (Implemented)
- :ref:`REQ-PLG-010` (Implemented)
- :ref:`REQ-PY-002` (Defect)
- :ref:`REQ-PY-011` (Defect)
- :ref:`REQ-PY-012` (Defect)
- :ref:`REQ-PY-020` (Defect)
- :ref:`REQ-PY-031` (Defect)
- :ref:`REQ-PY-040` (Defect)
- :ref:`REQ-PY-041` (Implemented)
- :ref:`REQ-PY-042` (Implemented)
- :ref:`REQ-PY-050` (Implemented)
- :ref:`REQ-PY-051` (Implemented)
- :ref:`REQ-PY-052` (Defect)
- :ref:`REQ-QRY-020` (Implemented)
- :ref:`REQ-TIME-001` (Implemented, Unspecified)
- :ref:`REQ-TIME-012` (Implemented, Unspecified)
- :ref:`REQ-TIME-013` (Defect)
- :ref:`REQ-TIME-033` (Defect)
- :ref:`REQ-URI-001` (Implemented)
- :ref:`REQ-URI-002` (Unspecified)
- :ref:`REQ-URI-003` (Defect)
- :ref:`REQ-URI-004` (Implemented)
- :ref:`REQ-URI-005` (Unspecified)
- :ref:`REQ-URI-006` (Defect)
- :ref:`REQ-URI-011` (Implemented)
- :ref:`REQ-URI-012` (Implemented)
- :ref:`REQ-URI-013` (Implemented)
- :ref:`REQ-URI-020` (Implemented)
- :ref:`REQ-URI-021` (Defect)
- :ref:`REQ-URI-023` (Defect)
- :ref:`REQ-URI-025` (Defect)
