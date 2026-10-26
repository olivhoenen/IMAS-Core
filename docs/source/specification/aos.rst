Arrays of structures
====================

An array of structures (AoS) is read or written through its own context.
``al_begin_arraystruct_action`` creates the context,
``al_iterate_over_arraystruct`` moves it from element to element, and
``al_end_action`` ends it. Data calls made with an AoS context address the
current element (see :doc:`data_access`). An AoS context can be the parent
of a nested AoS context.

The handle rules shared by all contexts are in :doc:`contexts`. Time slices
and time ranges are in :doc:`time`.

Creating an AoS context
-----------------------

.. _REQ-AOS-001:

REQ-AOS-001: Begin an array of structures action
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_arraystruct_action(ctx, path, timebase, &size,
            &aosctx)`` shall create an AoS context for the AoS ``path``.
            ``ctx`` is an operation context for a top-level AoS, or an AoS
            context for a nested AoS. ``path`` is relative to ``ctx``: to
            the IDS root or to the current element of the parent AoS.

            - **Write:** ``size`` is the number of elements the caller will
              write.
            - **Read:** on return, ``size`` is the number of stored
              elements. Its input value is ignored.

            ``timebase`` is empty for a static AoS. For a timed AoS it is
            the path of its time vector: each element is one time slice.
:Nominal: IMAS-Python passes ``size = 0`` for reads and ``len(aos)`` for
          writes, and opens one context per AoS level as it walks the
          structure.
:Errors: - Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in
           store".
         - Backend failure: ``BACKEND_ERR``.
:Status: Implemented (HDF5, Memory, MDSplus with DD paths). For per-backend
         defects, see :ref:`REQ-AOS-031`.
:Trace: ``src/al_lowlevel.cpp:589-616`` (``LLenv::create``,
        ``createAOS``), ``:1552-1638``. IMAS-Python
        ``imas/backends/imas_core/db_entry_helpers.py:55,110,183``. Contract
        tests ``Backends/AosMatrix.{TopLevelWriteIterateRead,NestedWriteIterateRead}``.

.. _REQ-AOS-002:

REQ-AOS-002: Context type check
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_begin_arraystruct_action`` shall refuse a ``ctx`` that is
            neither an operation context nor an AoS context. It shall fail
            with ``LOWLEVEL_ERR`` "Wrong Context type stored" and create no
            context.
:Status: *Defect* (from reading the code, not reproduced): a data entry
         context is passed to ``dynamic_cast<OperationContext*>``. That
         gives ``NULL``, and an AoS context is still built with a ``NULL``
         operation context and handed to the backend. Undefined behavior
         is expected.
:Trace: ``src/al_lowlevel.cpp:589-609``.

.. _REQ-AOS-003:

REQ-AOS-003: Empty array of structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When ``size`` is 0, after the backend call for a read or as
            given for a write, ``al_begin_arraystruct_action`` shall return
            status 0 and ``aosctx == 0``. It shall release everything it
            allocated. The caller then has nothing to iterate or end:
            ``al_end_action(0)`` is a no-op.
:Nominal: Reading an AoS that was never written gives ``size == 0`` and
          ``aosctx == 0``. IMAS-Python then skips the AoS.
:Status: *Defect* (resource leak): the backend action is ended and the
         context object is deleted, but its handle slot is never released.
         The handle table keeps a dangling pointer and does not shrink. In
         ``al_begin_arraystruct_action``, ``assert(actxID != 0)`` tests the
         output pointer instead of the handle.
:Trace: ``src/al_lowlevel.cpp:1283-1289`` (``al_plugin_begin_arraystruct_action``),
        ``:1601-1612``.

.. _REQ-AOS-004:

REQ-AOS-004: Negative size
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A negative ``size`` shall make ``al_begin_arraystruct_action``
            fail with ``LOWLEVEL_ERR`` "Returned size for array of
            structure is negative! (N)", leaving no context allocated.
:Errors: ``LOWLEVEL_ERR``.
:Status: *Defect* (resource leak): the check runs after the backend call
         and after the handle is allocated, so the handle and the
         backend's AoS action stay open. On a write, the negative size
         reaches the backend first.
:Trace: ``src/al_lowlevel.cpp:1290-1298,1614-1618``.

.. _REQ-AOS-005:

REQ-AOS-005: Plugins and AoS contexts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When plugins are bound to ``path``, exactly one of them shall
            create the AoS context. If none does, the call fails with
            ``LOWLEVEL_ERR`` "No AOS context has been created, or one
            plugin has removed it at path: ...". If two or more do, it fails
            with ``LOWLEVEL_ERR`` "... only one plugin is allowed to create
            an AOS context at a given path", listing the plugins.
:Limits: When the plugin framework is enabled, an AoS opened at the special
         get-operation path in a ``WRITE_OP`` context gets ``size = 0`` and
         is skipped.
:Status: Implemented. See :doc:`plugins`.
:Trace: ``src/al_lowlevel.cpp:1560-1593``;
        ``src/access_layer_plugin_manager.cpp:524-532``.

Iterating
---------

.. _REQ-AOS-010:

REQ-AOS-010: Current element and iteration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: An AoS context shall start at element 0.
            ``al_iterate_over_arraystruct(aosctx, step)`` shall add
            ``step`` to the current index; ``step`` may be positive, zero or
            negative. Reads, writes and nested AoS actions made with
            ``aosctx`` shall address the element at the current index.
:Nominal: A sequential walk calls ``step = 1`` after each element. For lazy
          loading, IMAS-Python jumps to any element with ``step = index -
          current``, which can be negative, while the context stays open.
:Limits: - The index is not checked against ``[0, size)``.
         - Moving outside that range and then reading or writing gives a
           backend-dependent result.
:Errors: Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in store".
:Status: Implemented for in-range indices. *Unspecified*: whether moving
         out of range should be refused (for example with
         ``CONTEXT_ERR``).
:Trace: ``src/al_context.cpp:563-566``; ``include/al_context.h:505``;
        ``src/al_lowlevel.cpp:1321-1346``. IMAS-Python
        ``imas/backends/imas_core/al_context.py:215-229``; test
        ``imas/test/test_lazy_loading.py`` (random-order access).

.. _REQ-AOS-011:

REQ-AOS-011: Iterating a handle that is not an AoS context
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_iterate_over_arraystruct`` called with a valid handle that
            is not an AoS context shall fail with ``LOWLEVEL_ERR`` "Wrong
            Context type stored", and change nothing.
:Status: *Defect* (memory corruption): the handle's context is
         ``static_cast`` to ``ArraystructContext`` without a check. The
         index update then writes into the memory of a data entry or
         operation context.
:Trace: ``src/al_lowlevel.cpp:1329-1331``.

Ending
------

.. _REQ-AOS-020:

REQ-AOS-020: Ending an AoS context
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_end_action(aosctx)`` shall end the AoS action and release
            its handle. For a write, the backend may flush the AoS when it
            ends. A nested AoS context shall be ended before its parent,
            and every AoS context before its operation context.
:Limits: MDSplus reports errors on AoS writes only at ``al_end_action``,
         for example ``%TREE-W-NNF`` for a path that is not in its model.
:Status: Implemented. What happens when a parent is ended first is
         specified in :doc:`contexts`.
:Trace: ``src/al_lowlevel.cpp:1640-1675``.

Time and backends
-----------------

.. _REQ-AOS-030:

REQ-AOS-030: Timed AoS in slice and time range reads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a slice read, a timed AoS that is not empty shall report
            ``size == 1``. Its single element is the one selected by the
            interpolation mode at the requested time (:ref:`REQ-TIME-010`).
            In a time range read, a timed AoS that is not empty shall report
            one element per returned time: the samples in ``[tmin, tmax]``,
            or the resampled times (:ref:`REQ-TIME-030`). A static AoS is
            returned whole in both cases.
:Nominal: Slice read of ``equilibrium/time_slice`` gives one
          ``time_slice`` element.
:Limits: - On Memory, a ``LINEAR_INTERP`` slice interpolates the whole
           element between its two neighbours.
         - MDSplus also interpolates whole structures in slice reads.
:Status: Implemented (HDF5, Memory, MDSplus).
:Trace: ``src/hdf5/hdf5_reader.cpp:258-265``;
        ``src/memory_backend.cpp:695-730``. Contract tests (PR #96):
        ``MdsplusStructAwareSliceInterpolation*``.

.. _REQ-AOS-031:

REQ-AOS-031: Backend support for arrays of structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Every backend that stores data shall read back an AoS with the
            size and element content that were written.
:Status: - *Defect*: ASCII always reports ``size == 0`` when reading an
           AoS.
         - *Defect*: UDA returns the empty value for a dynamic leaf inside
           an AoS.
         - *Divergence*: Flexbuffers refuses every read in a serialization
           session.
         - *Divergence*: MDSplus only accepts DD paths, and reports
           unknown AoS paths at ``al_end_action``.
:Trace: ``src/ascii_backend.cpp:213-235,656-682``. Contract tests
        ``AosKnownDefects.DISABLED_AsciiAosReadReportsWrittenSize``, the
        UDA AoS rows in ``tests/contract/TRACEABILITY.md`` (PR #97), and
        ``Backends/AosMatrix.*/Flexbuffers``.
