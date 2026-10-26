Contexts and handles
====================

Every object that IMAS-Core keeps for the caller (a data entry, an
operation or an array of structures) is a *context*, identified by an
integer *handle*. This page specifies how handles are given out, checked
and released. The context types are described in :doc:`architecture`.

Handles
-------

.. _REQ-CTX-001:

REQ-CTX-001: Handle values
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Every successful ``al_begin_dataentry_action``,
            ``al_begin_global_action``, ``al_begin_slice_action``,
            ``al_begin_timerange_action`` and ``al_begin_arraystruct_action``
            (with a non-zero size) shall return a strictly positive handle
            that differs from the handles of all live contexts in the
            process. Handle ``0`` shall never identify a context.
:Nominal: Handles are small integers, shared by all context types and all
          data entries of the process.
:Limits: After a handle is ended, the same number may be given to a new
         context (see :ref:`REQ-CTX-004`).
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:39`` (slot 0 is reserved), ``:537-556``
        (``addLLenv``).

.. _REQ-CTX-002:

REQ-CTX-002: Handle 0
^^^^^^^^^^^^^^^^^^^^^

:Statement: Handle ``0`` means "no context". ``al_end_action(0)`` shall
            succeed and do nothing, and ``al_context_info(0, &info)`` shall
            succeed with the text ``NULL context``. Every other function
            shall fail on handle ``0`` as on an unknown handle
            (:ref:`REQ-CTX-003`).
:Nominal: ``al_begin_arraystruct_action`` returns handle 0 for an empty
          array of structures (see :doc:`aos`); the caller can pass it to
          ``al_end_action`` without checking.
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:801-811``, ``:1640-1644``. Test
        ``ContextInfo.NullContextReturnsLiteralString`` (PR #95).

.. _REQ-CTX-003:

REQ-CTX-003: Unknown or ended handle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A function called with a handle that was never given out, or
            that was already ended, shall fail with ``LOWLEVEL_ERR`` and the
            message ``Cannot find context <n> in store``, and shall have no
            other effect.
:Nominal: Reading through an operation handle after ``al_end_action`` on it
          fails cleanly.
:Limits: Negative handles and handles beyond the table are unknown handles.
:Errors: As stated.
:Status: Defect for ``al_end_action``, crash. All other functions behave as
         stated. ``al_end_action`` removes the handle from the table before
         using it:

         - On a handle beyond the table it fails with ``UNKNOWN_ERR``
           (``std::out_of_range``) instead of ``LOWLEVEL_ERR``.
         - On a handle that was already ended (a double end), it calls the
           backend through a null pointer and the process crashes.
:Trace: ``src/al_lowlevel.cpp:558-572`` (``getLLenv``), ``:574-587``
        (``delLLenv``), ``:1640-1675``. Test
        ``UdaBreadthTest.AosTraversalAdvancesAcrossDistinctStaticElements``
        checks that iterating an ended context fails (PR #97).

.. _REQ-CTX-004:

REQ-CTX-004: Handle reuse
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A handle number may be given to a new context once the context
            it identified has been ended. Callers shall not use a handle
            after ending it.
:Limits: Handles are reused last-in, first-out: only the most recently
         given number is reused when it is ended first. Numbers freed out of
         order are never reused, so the table only grows in a long-running
         process (by two pointers per handle).
:Errors: A stale handle that was reused identifies the new context, and a
         call through it acts on that context. IMAS-Core cannot detect this.
:Status: Implemented. Detecting stale handles (for example with a
         generation counter in the handle) would need an API decision; see
         :ref:`spec-open-questions`.
:Trace: ``src/al_lowlevel.cpp:537-587``.

Context types and nesting
-------------------------

.. _REQ-CTX-010:

REQ-CTX-010: Context tree
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Contexts shall be created in this hierarchy: an operation from
            a data entry handle, an array of structures from an operation
            handle or from another array of structures handle.
:Nominal: data entry → global operation → ``time_slice`` AoS →
          ``profiles_2d`` AoS.
:Errors: See :ref:`REQ-CTX-011`.
:Status: Implemented.
:Trace: ``include/al_context.h``; ``src/al_lowlevel.cpp:589-616``
        (``LLenv::create``).

.. _REQ-CTX-011:

REQ-CTX-011: Context type checks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Every function shall check that the handle it receives has the
            expected context type, and fail with ``LOWLEVEL_ERR``
            ``Wrong Context type stored`` otherwise.

            .. list-table::
               :header-rows: 1
               :widths: 50 50

               * - Function
                 - Expected handle
               * - ``al_close_pulse``, ``al_begin_global_action``,
                   ``al_begin_slice_action``, ``al_begin_timerange_action``,
                   ``al_get_backendID``, ``al_get_occurrences``,
                   ``al_list_filled_paths``
                 - data entry
               * - ``al_delete_data``
                 - operation
               * - ``al_begin_arraystruct_action``, ``al_read_data``,
                   ``al_write_data``
                 - operation or array of structures
               * - ``al_iterate_over_arraystruct``
                 - array of structures
               * - ``al_end_action``, ``al_context_info``
                 - any
:Status: Defect, crash or wrong result. The check is made for
         ``al_close_pulse``, the three ``al_begin_*_action`` functions on a
         data entry and ``al_delete_data``. It is missing for:

         - ``al_get_backendID`` and ``al_iterate_over_arraystruct``, which
           use ``static_cast``. Iterating a non-AoS handle writes into an
           unrelated object.
         - ``al_read_data`` and ``al_write_data``, which pass any context to
           the backend.
         - ``al_get_occurrences`` and ``al_list_filled_paths``, which pass
           any context to the backend.
:Trace: ``src/al_lowlevel.cpp:845`` (``al_get_backendID``), ``:1329``
        (``al_iterate_over_arraystruct``), ``:1144-1239``, ``:1753-1800``.
        Tests ``GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError``
        and tripwire
        ``GetBackendIdKnownDefects.WrongContextTypeCurrentlySucceedsViaUnsafeCast``
        (PR #95).

.. _REQ-CTX-012:

REQ-CTX-012: Ending order
^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The caller shall end contexts in the reverse order of their
            creation: arrays of structures before their operation,
            operations before their data entry. What IMAS-Core does when a
            context is ended while it still has live children is not
            defined yet.
:Nominal: IMAS-Python keeps nested contexts in a stack and ends them last
          in, first out.
:Status: Unspecified. Currently the parent is deleted (for a data entry,
         together with its backend object) and the children keep dangling
         pointers to it; any later use of a child, including
         ``al_end_action``, is undefined behavior. A safe definition is
         either to refuse with ``CONTEXT_ERR`` or to end the children
         first.
:Trace: ``include/al_context.h`` (raw ``pctx``, ``opctx``, ``parent``
        pointers); ``src/al_lowlevel.cpp:1648-1655``. IMAS-Python
        ``imas/backends/imas_core/al_context.py:232-415``.

.. _REQ-CTX-013:

REQ-CTX-013: Several operations on one data entry
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Whether several operations may be open at the same time on one
            data entry (for example reading two IDSs in an interleaved way,
            or reading one IDS while writing another) is not defined yet.
:Status: Unspecified. The API allows it, but backends keep per-entry state
         (caches cleared at ``al_end_action``, a single current file in
         ASCII, a single builder in Flexbuffers). IMAS-Python ends all
         other open contexts before each get, put or delete because "mixing
         contexts can be problematic" (IMAS-5603).
:Trace: IMAS-Python ``imas/backends/imas_core/db_entry_al.py:183, 241,
        281, 343``; ``imas/backends/imas_core/al_context.py:343``.

Ending a context
----------------

.. _REQ-CTX-020:

REQ-CTX-020: al_end_action
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_end_action(ctx)`` shall, in this order: tell the bound
            plugins (when plugins are enabled), let the backend finish the
            context (for a write operation, this may flush data to
            storage), delete the context, delete the backend object if
            ``ctx`` is a data entry handle, and release the handle.
:Nominal: Ending a write operation on MDSplus writes the buffered data;
          errors found then (for example a path missing from the MDSplus
          model) are reported by ``al_end_action``, not by
          ``al_write_data``.
:Errors: A failure reported by the backend shall be returned with its
         status code (see :ref:`REQ-ERR-004` for the current code), and the
         handle shall still be released.
:Status: Defect, resource leak. The handle is removed from the table before
         the backend is called, so when the backend throws, the context
         (and, for a data entry, the backend object) is never deleted and
         cannot be ended again.
:Trace: ``src/al_lowlevel.cpp:1640-1675``.
