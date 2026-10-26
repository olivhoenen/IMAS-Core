Behavior relied on by IMAS-Python
=================================

IMAS-Python is the main user of the ``imas_core`` Python package. Its
``imas/backends/imas_core/`` modules call the low-level API, and several of
its features depend on IMAS-Core behaving in a particular way. Changing
any behavior listed here breaks IMAS-Python, or forces it to change, so
these requirements need extra care.

The table maps each assumption to its requirement. Paths are relative to
the IMAS-Python repository; ``imas_core/`` stands for
``imas/backends/imas_core/``.

Calls and feature detection
---------------------------

.. list-table::
   :header-rows: 1
   :widths: 45 25 30

   * - Assumption
     - Requirements
     - Where in IMAS-Python
   * - ``get_al_version()`` returns a version that ``packaging.Version``
       can parse. Features are enabled by version: AL 5.0 (URIs, plugins),
       5.1 (``al_get_occurrences``), 5.3 (Flexbuffers), 5.4
       (``al_begin_timerange_action``), 5.5.1 (UDA ``cache_mode=none``),
       5.6 (``MDSPLUS_MODELS_PATH``), 5.7 (``al_list_filled_paths``).
     - :ref:`REQ-QRY-020`, :ref:`REQ-PY-001`, :ref:`REQ-NFR-030`
     - ``imas_core/imas_interface.py:68-80``, ``imas/ids_defs.py:128-130``,
       ``imas_core/mdsplus_model.py:298-309``
   * - ``imas_core_config_enable_exceptions()`` exists (looked up with
       ``getattr``) and makes the context functions raise.
     - :ref:`REQ-PY-011`
     - ``imas_core/imas_interface.py:23-25``
   * - ``al_begin_*_action`` return ``(status, ctx)`` tuples, but
       ``al_begin_timerange_action`` returns a bare context.
     - :ref:`REQ-PY-020`
     - ``imas_core/al_context.py:74-121``
   * - ``ALException`` and the ``imas_core.exception`` classes are exported;
       a backend that is not built raises ``ImasCoreBackendException`` with
       ``not available`` in its message.
     - :ref:`REQ-ERR-021`, :ref:`REQ-PY-010`, :ref:`REQ-URI-011`,
       :ref:`REQ-BE-001`
     - ``imas/exception.py:24``, ``imas/test/conftest.py:61-67``
   * - The constants of ``imasdef`` keep their values (backend IDs, modes,
       data types, ``EMPTY_*``, ``IDS_TIME_MODE_*``).
     - :ref:`REQ-PY-002`
     - ``imas/ids_defs.py:96-131``, ``imas/ids_data_type.py:46-60``

Data entries and operations
---------------------------

.. list-table::
   :header-rows: 1
   :widths: 45 25 30

   * - Assumption
     - Requirements
     - Where in IMAS-Python
   * - Modes ``r``, ``a``, ``w`` and ``x`` map to ``OPEN_PULSE``,
       ``FORCE_OPEN_PULSE``, ``FORCE_CREATE_PULSE`` and ``CREATE_PULSE``.
     - :ref:`REQ-DE-002` to :ref:`REQ-DE-005`
     - ``imas_core/db_entry_al.py:45-62``
   * - Legacy parameters, including the full DD version string, are turned
       into a URI by ``al_build_uri_from_legacy_parameters``.
     - :ref:`REQ-URI-022`, :ref:`REQ-URI-023`
     - ``imas_core/db_entry_al.py:105``
   * - Query options are separated by ``&``, ``;`` or ``?``.
     - :ref:`REQ-URI-004`
     - ``imas_core/db_entry_al.py:72-77``
   * - Closing is ``al_close_pulse`` then ``al_end_action``;
       ``close(erase=True)`` passes ``ERASE_PULSE``.
     - :ref:`REQ-DE-020`, :ref:`REQ-DE-021`
     - ``imas_core/db_entry_al.py:155-159``
   * - Occurrence 0 is addressed without a suffix; an invalid occurrence
       such as ``core_profiles/invalid`` reads as empty instead of failing.
     - :ref:`REQ-OP-003`
     - ``imas_core/db_entry_al.py:186-188``, ``imas/test/test_get_sample.py``
   * - UDA global reads pass ``datapath="ids_properties"`` to read the
       header only.
     - :ref:`REQ-OP-011`, :ref:`REQ-BE-052`
     - ``imas_core/db_entry_al.py:190, 248``
   * - ``put_slice`` begins a slice operation with ``UNDEFINED_TIME`` and
       ``UNDEFINED_INTERP``.
     - :ref:`REQ-OP-020`, :ref:`REQ-TIME-020`
     - ``imas_core/db_entry_al.py:328-330``
   * - ``get_sample`` gets a ``ValueError`` from IMAS-Core for
       ``tmin > tmax`` and for ``dtime`` without an interpolation mode.
     - :ref:`REQ-OP-031`, :ref:`REQ-PY-021`, :ref:`REQ-TIME-030`
     - ``imas/test/test_get_sample.py:108-114``
   * - Nested contexts are ended in reverse order (a LIFO stack); contexts
       of another IDS are ended before a new operation starts
       (IMAS-5603).
     - :ref:`REQ-CTX-012`, :ref:`REQ-CTX-013`
     - ``imas_core/al_context.py:232-415``

Data
----

.. list-table::
   :header-rows: 1
   :widths: 45 25 30

   * - Assumption
     - Requirements
     - Where in IMAS-Python
   * - Missing data is not an error: a missing array reads as ``None``, a
       missing scalar as its ``EMPTY_*`` value and a missing string as
       ``""``.
     - :ref:`REQ-DATA-021`, :ref:`REQ-PY-040`
     - ``imas_core/db_entry_helpers.py:68-77, 119-130``
   * - ``ids_properties/homogeneous_time`` reads as a value outside
       ``IDS_TIME_MODES`` when the IDS does not exist; this is how
       IMAS-Python detects an absent IDS.
     - :ref:`REQ-DATA-021`
     - ``imas_core/db_entry_al.py:249-258``
   * - Empty values are not written, and ``put`` first deletes every leaf
       and AoS of the IDS, then writes the filled nodes.
     - :ref:`REQ-DATA-011`, :ref:`REQ-DATA-030`
     - ``imas_core/db_entry_al.py:320-325``,
       ``imas_core/db_entry_helpers.py:133-139``
   * - ``put`` can be repeated on an open data entry.
     - :ref:`REQ-DATA-012`
     - ``imas/db_entry.py``
   * - Strings are read with one more dimension than the node (rank 1 for
       a string, rank 2 for a string array).
     - :ref:`REQ-DATA-003`, :ref:`REQ-PY-040`
     - ``imas/ids_metadata.py:257-258``
   * - Timebase paths are relative to the AoS context, and ``"/time"`` is
       used for all dynamic nodes of a homogeneous IDS.
     - :ref:`REQ-DATA-004`, :ref:`REQ-TIME-001`
     - ``imas/ids_metadata.py:190-203``,
       ``imas_core/db_entry_helpers.py:50, 106, 171``
   * - ``version_put/data_dictionary`` is stored and returned unchanged;
       all DD version conversion is done in Python.
     - :ref:`REQ-QRY-021`, :ref:`REQ-NFR-031`
     - ``imas/db_entry.py:574-581``

Arrays of structures, time and queries
--------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 45 25 30

   * - Assumption
     - Requirements
     - Where in IMAS-Python
   * - An AoS read with size 0 returns the stored size; size 0 returns
       context 0.
     - :ref:`REQ-AOS-001`, :ref:`REQ-AOS-003`
     - ``imas_core/db_entry_helpers.py:55, 110, 183``
   * - Lazy loading moves the AoS cursor with relative and negative steps
       on a context that stays open.
     - :ref:`REQ-AOS-010`, :ref:`REQ-PY-050`
     - ``imas_core/al_context.py:215-229``,
       ``imas/test/test_lazy_loading.py``
   * - Time range bounds are inclusive; ``CLOSEST_INTERP`` picks the later
       sample on a tie; a resampled IDS reads back as homogeneous.
     - :ref:`REQ-TIME-011`, :ref:`REQ-TIME-030`, :ref:`REQ-TIME-031`
     - ``imas/test/test_get_sample.py``
   * - ``al_get_occurrences`` returns the occurrences in increasing order,
       or ``None`` when there are none.
     - :ref:`REQ-QRY-001`, :ref:`REQ-PY-041`
     - ``imas_core/al_context.py:168-175``,
       ``imas/test/test_list_occurrences.py``
   * - ``al_list_filled_paths`` returns paths without AoS indices, including
       the ``version_put`` fields.
     - :ref:`REQ-QRY-002`
     - ``imas_core/db_entry_al.py:367-377``,
       ``imas/test/test_list_filled_paths.py``

Backends
--------

.. list-table::
   :header-rows: 1
   :widths: 45 25 30

   * - Assumption
     - Requirements
     - Where in IMAS-Python
   * - IMAS-Python builds the MDSplus model tree itself when the mode is
       not ``OPEN_PULSE`` and checks ``maxoccur`` for MDSplus.
     - :ref:`REQ-BE-030`, :ref:`REQ-BE-032`
     - ``imas_core/mdsplus_model.py``, ``imas_core/db_entry_al.py:333``
   * - UDA needs ``IDSDEF_PATH`` to point to the DD of the data;
       ``cache_mode=none`` or ``fetch=1`` avoids a stale cache when the DD
       versions differ (IMAS-Python GH#97).
     - :ref:`REQ-BE-051`, :ref:`REQ-BE-052`, :ref:`REQ-BE-053`
     - ``imas_core/uda_support.py``, ``imas_core/db_entry_al.py:260-273``
   * - Two data entries on the same Memory path share state; IMAS-Python
       tests skip the second ``close()`` to avoid a double free.
     - :ref:`REQ-DE-030`, :ref:`REQ-BE-071`
     - ``imas/test/test_minimal_io.py:20``,
       ``imas/test/test_nbc_change.py:91``
   * - The Memory backend does not support ``al_get_occurrences``.
     - :ref:`REQ-BE-073`
     - ``imas/test/test_list_occurrences.py``
   * - ASCII supports neither slices nor lazy loading; serialization uses
       the ``filename`` option.
     - :ref:`REQ-BE-080`, :ref:`REQ-BE-081`
     - ``imas_core/db_entry_al.py:178-179``, ``imas/ids_toplevel.py:46-53``
   * - Flexbuffers serializes through the ``<buffer>`` field, read with
       ``al_read_data_array`` and written with the private
       ``_al_write_data_array``.
     - :ref:`REQ-BE-090`, :ref:`REQ-PY-042`
     - ``imas/ids_toplevel.py:35, 166, 203``
