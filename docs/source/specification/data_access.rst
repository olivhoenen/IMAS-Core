Reading, writing and deleting data
==================================

This page specifies ``al_write_data``, ``al_read_data`` and ``al_delete_data``:
the data types and ranks they accept, how buffers are laid out and owned,
how missing and empty data are represented, and what a delete removes.

These functions act on an operation context (see :doc:`operations`) or an
array of structures context (see :doc:`aos`). How handles are validated is
specified in :doc:`contexts`, and how status codes are built in
:doc:`errors`. When a plugin is bound to the path, the call is passed to the
plugin instead of the backend (see :doc:`plugins`). The requirements below
describe the case without a plugin.

Data model and buffers
----------------------

.. _REQ-DATA-001:

REQ-DATA-001: Data types and ranks
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: IMAS-Core shall exchange data of four types, ``CHAR_DATA`` (50),
            ``INTEGER_DATA`` (51), ``DOUBLE_DATA`` (52) and ``COMPLEX_DATA``
            (53), with a rank ``dim`` from 0 (scalar) to ``MAXDIM`` (7).
:Nominal: ``INTEGER_DATA``, ``DOUBLE_DATA`` and ``COMPLEX_DATA`` at every
          rank from 0 to 7, and ``CHAR_DATA`` at rank 1 (a string) and 2 (a
          list of strings), write and then read back the same value and
          shape on HDF5, Memory and ASCII.
:Limits: - ``CHAR_DATA`` with a rank above 2 is refused by HDF5 and ASCII
           (``BACKEND_ERR``, ASCII message "CHAR data > 2D is not
           implemented yet"). Memory accepts it.
         - The core does not check ``dim`` against ``MAXDIM``. A rank
           above 7 is passed to the backend, and the result depends on the
           backend (ASCII: ``BACKEND_ERR`` "... data > 7D is not
           implemented").
         - MDSplus and UDA only store paths that exist in the Data
           Dictionary, so the ranks they accept are those of the DD nodes
           (see :doc:`backends`).
:Errors: An unknown ``datatype`` is refused by the backend, e.g. HDF5
         ``BACKEND_ERR`` "Data type not supported". On read, the core
         raises ``LOWLEVEL_ERR`` "Unknown data type=N" when it has to fill
         in or convert a value of an unknown type.
:Status: *Defect* for some cells, all pinned by contract tests:

         - HDF5 crashes on a ``CHAR_DATA`` scalar (rank 0).
         - ASCII corrupts ``DOUBLE_DATA`` and ``COMPLEX_DATA`` at rank 7,
           and aborts on ``INTEGER_DATA`` at rank 7.

         *Unspecified*: whether the core should refuse ``dim > MAXDIM``
         itself. The developer guide still says arrays have "dimensions
         from 1 to 6".
:Trace: ``include/al_defs.h.in:4`` (``MAXDIM``);
        ``src/ascii_backend.cpp:367,452``;
        ``src/hdf5/hdf5_dataset_handler.cpp:518``;
        ``docs/source/developers/development.rst:47``. Contract tests
        (PR #95): ``Backends/RoundTripMatrix.ReadEqualsWrite/*``,
        ``RoundTripKnownDefects.DISABLED_Hdf5CharScalarRoundTrips``,
        ``RoundTripKnownDefects.DISABLED_Ascii{Integer,Double,Complex}MaxdimRoundTrips``.

.. _REQ-DATA-002:

REQ-DATA-002: Buffer layout
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A data buffer of rank ``dim > 0`` shall be a contiguous array in
            column-major (Fortran) order: ``size[0]`` is the
            fastest-varying dimension. The element types are C ``char``,
            ``int`` (32 bits), ``double`` and ``std::complex<double>``
            (two ``double``). For dynamic data, the time dimension is the
            last one (``size[dim-1]``).
:Nominal: The Python binding converts every array to Fortran order before
          writing, and wraps read buffers as Fortran-ordered numpy arrays.
:Limits: ``size`` may be ``NULL`` when ``dim`` is 0.
:Status: Implemented.
:Trace: ``python/imas_core/_al_lowlevel.pyx:631`` (``asfortranarray``),
        ``:912-914`` (Fortran-order read view);
        ``src/data_interpolation.cpp:349-350`` (the last dimension is time).

.. _REQ-DATA-003:

REQ-DATA-003: Strings
^^^^^^^^^^^^^^^^^^^^^

:Statement: A string shall be exchanged as ``CHAR_DATA`` of rank 1, with
            ``size[0]`` its length in bytes. No terminating NUL is needed.
            A list of strings shall be ``CHAR_DATA`` of rank 2 with
            ``size = {n, L}``, given as ``n`` consecutive records of ``L``
            bytes. ``L`` is the longest string, and shorter strings are
            padded with NUL bytes. A ``CHAR_DATA`` scalar is a single byte.
:Nominal: The Python binding writes ``str`` as rank 1 and ``list[str]`` as
          rank 2. When reading, it strips the trailing NUL padding of each
          record.
:Limits: - An empty string has ``size[0] == 0``, so it is not written (see
           :ref:`REQ-DATA-011`). Reading it back gives the empty string.
         - A list of strings is written even when ``n`` or ``L`` is 0,
           because the zero-size check is skipped for ``CHAR_DATA`` of rank
           2 and above (IMAS-4948).
         - Strings are bytes for the core. The Python binding encodes and
           decodes UTF-8 (see :doc:`python_binding` for its known
           byte-length issue).
:Status: *Defect*: HDF5 crashes on a ``CHAR_DATA`` scalar (see
         :ref:`REQ-DATA-001`). Otherwise implemented.
:Trace: ``python/imas_core/_al_lowlevel.pyx:44-57,60-75,694-748``;
        ``src/al_lowlevel.cpp:759-792`` (``data_has_non_zero_shape``).

.. _REQ-DATA-004:

REQ-DATA-004: Field and timebase paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``fieldpath`` and ``timebasepath`` arguments shall be
            ``/``-separated node paths, relative to the context they are
            used with: the IDS root for an operation context, or the current
            element for an array of structures context. A path that starts
            with ``/`` shall be resolved from the IDS root, whatever the
            context. An empty ``timebasepath`` means the data is static.
:Nominal: IMAS-Python passes paths relative to the innermost AoS, and
          ``"/time"`` as the timebase of every dynamic node of a homogeneous
          IDS.
:Limits: IMAS-Core does not check paths against the Data Dictionary and
         stores any path it receives (HDF5, Memory, ASCII). MDSplus and UDA
         only accept DD paths (see :doc:`backends`).
:Status: *Defect* on the baseline: HDF5 reads ignore the leading ``/`` of
         an absolute path inside an AoS context (see
         :ref:`REQ-DATA-024`).
:Trace: ``include/al_lowlevel.h:370-413`` (parameter documentation);
        IMAS-Python ``imas/backends/imas_core/db_entry_helpers.py:49-51``
        (``"/time"``).

Writing
-------

.. _REQ-DATA-010:

REQ-DATA-010: Write and read back
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_write_data(ctx, fieldpath, timebasepath, data, datatype,
            dim, size)`` shall store the value at ``fieldpath`` in the IDS
            occurrence of ``ctx``. Once the write action has ended,
            ``al_read_data`` with the same path, type and rank in a read
            action shall return the same value and shape.
:Nominal: Status code 0. IMAS-Core does not keep a reference to ``data``
          after the call; the caller still owns it.
:Limits: - Writing in a ``READ_OP`` context is not checked by the core. The
           result depends on the backend.
         - Writing the same path twice in one action is not specified.
           Writing a path in a new ``WRITE_OP`` action replaces the
           previous value (see :ref:`REQ-DATA-012`).
:Errors: - Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in
           store".
         - Backend failure: ``BACKEND_ERR``, with the backend's message.
         - Some backends report write errors only at ``al_end_action``.
           MDSplus does this for timed and AoS data.
:Status: Implemented. *Unspecified*: whether writes in a ``READ_OP``
         context should be refused.
:Trace: ``src/al_lowlevel.cpp:1144-1175,1677-1712``. Contract tests
        (PR #95): ``Backends/RoundTripMatrix.*``, ``RoundTrip.*``.

.. _REQ-DATA-011:

REQ-DATA-011: Empty values are not written
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When no plugin is bound to the path, ``al_write_data`` shall
            do nothing and return status 0 if the data is empty. Data is
            empty when:

            - ``data`` is ``NULL``;
            - it is an ``INTEGER_DATA``, ``DOUBLE_DATA`` or ``COMPLEX_DATA``
              scalar equal to ``EMPTY_INT``, ``EMPTY_DOUBLE`` or
              ``EMPTY_COMPLEX``;
            - ``dim > 0`` and ``size`` is ``NULL``;
            - or some ``size[i]`` is 0, except for ``CHAR_DATA`` of rank 2
              and above.
:Nominal: An HLI can send every node of an IDS, and unset nodes leave no
          trace in the data entry.
:Limits: - A ``CHAR_DATA`` scalar is always written, even ``EMPTY_CHAR``.
         - Writing an empty value does not delete an older value at the
           same path. To clear it, the HLI deletes it first; IMAS-Python's
           ``put()`` does so (see :ref:`REQ-DATA-030`).
         - When a plugin is bound, empty data is passed to the plugin (see
           :doc:`plugins`).
         - The Python binding also returns 0 without calling the core for
           an array with a zero-length dimension.
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:759-792,1690-1693``;
        ``python/imas_core/_al_lowlevel.pyx:607-608,756-759``.

.. _REQ-DATA-012:

REQ-DATA-012: Rewriting an IDS occurrence
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A data entry kept open shall accept any number of successive
            ``WRITE_OP`` global actions on the same IDS occurrence. Each
            rewritten path shall hold the last value written, and paths not
            rewritten shall keep their value.
:Nominal: IMAS-Python calls ``put()`` repeatedly on the same open entry.
:Status: *Defect* on the baseline: on HDF5, the second whole-IDS write on
         the same open pulse fails with ``BACKEND_ERR`` "Unable to create
         HDF5 dataset", because the dataset is created again (issue #60).
         Fixed by pending PR #100.
:Trace: ``src/hdf5/hdf5_writer.cpp`` (``write_ND_Data``). Contract tests
        (PR #100): ``EquilibriumSeedMatrix.RepeatedWholeIdsWriteOnSameOpenPulseSucceeds``,
        ``…WholeIdsReadThenTimeSliceElementReadSucceeds``.

Reading
-------

.. _REQ-DATA-020:

REQ-DATA-020: Read result and buffer ownership
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_read_data(ctx, fieldpath, timebasepath, &data, datatype,
            dim, size)`` shall return the stored value as follows:

            - **Scalar** (``dim == 0``): ``*data`` shall point to storage
              owned by the caller, which is large enough for one value of
              ``datatype``. IMAS-Core copies the value there.
            - **Array** (``dim > 0``): IMAS-Core sets ``*data`` to a new
              buffer allocated with ``malloc``, which the caller releases
              with ``free``. It fills ``size[0..dim-1]``; ``size`` shall
              have room for ``dim`` integers.
:Nominal: The Python binding wraps array buffers without copying them and
          frees them with the numpy array.
:Limits: ``size[i]`` for ``i >= dim`` is not touched.
:Errors: Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in store".
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:618-643`` (``setValue``),
        ``:1177-1239``; ``python/imas_core/_al_lowlevel.pyx:823-918``.

.. _REQ-DATA-021:

REQ-DATA-021: Missing data
^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the backend has no data at the path, ``al_read_data``
            shall return status 0 with:

            - a scalar set to ``EMPTY_INT``, ``EMPTY_DOUBLE``,
              ``EMPTY_COMPLEX`` or ``EMPTY_CHAR``;
            - for an array, ``*data == NULL`` and ``size[i] == 0`` for every
              ``i < dim``.
:Nominal: Reading a node that was never written, in an IDS occurrence that
          exists or not, gives the empty value. IMAS-Python reads
          ``ids_properties/homogeneous_time`` and treats ``EMPTY_INT`` as
          "this IDS occurrence is empty".
:Limits: - A stored ``EMPTY_*`` value cannot be told apart from missing
           data. With :ref:`REQ-DATA-011` such a value is never stored
           anyway.
         - An HDF5 slice read with ``LINEAR_INTERP`` reports a node as
           missing, and prints a warning on standard output, when
           interpolation is impossible (see :ref:`REQ-TIME-013`).
:Status: *Divergence*:

         - MDSplus raises an error (``%TREE-W-NNF``) for a path that is not
           in its model.
         - UDA fails on an IDS that was never written, while an unwritten
           leaf of a written IDS gives the empty value.
:Trace: ``src/al_lowlevel.cpp:645-677`` (``setDefaultValue``),
        ``:1190-1199``. IMAS-Python
        ``imas/backends/imas_core/db_entry_helpers.py:68-77,117-130``,
        ``db_entry_al.py:249-258``. Python binding: an array returns
        ``None``, a string returns ``''``
        (``_al_lowlevel.pyx:891-895,977-978``).

.. _REQ-DATA-022:

REQ-DATA-022: Rank mismatch
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the stored rank differs from the requested ``dim``,
            ``al_read_data`` shall fail with ``LOWLEVEL_ERR``. The message
            names both: "Wrong dimension of Data returned by backend:
            expected <type> in <dim>D but got <type> in <dim>D".
:Errors: ``LOWLEVEL_ERR``. ``*data`` and ``size`` are not set.
:Status: Implemented. The message is also printed on standard output with
         a ``-->status.message =`` prefix, which is a debugging leftover
         (see :doc:`errors`).
:Trace: ``src/al_lowlevel.cpp:1200-1207,1231``.

.. _REQ-DATA-023:

REQ-DATA-023: Type mismatch
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the stored type differs from the requested ``datatype``
            and the ranks match, ``al_read_data`` converts the value with
            a C cast to the requested type. It returns status 0 and puts a
            warning in ``status.message``: "Warning: <uri>/<field>
            returned with type <T1> while we expect type <T2>".
:Nominal: An integer node stored as ``DOUBLE_DATA`` reads back as a
          truncated ``int``.
:Limits: - Conversions may lose data (``DOUBLE`` to ``INTEGER`` or
           ``CHAR`` truncates, and out-of-range values are undefined in
           C++).
         - Stored ``COMPLEX_DATA`` cannot be converted. The result is the
           missing-data value of :ref:`REQ-DATA-021`.
         - This is the only case where ``status.message`` holds text while
           the code is 0. Callers shall not rely on ``status.message``
           when the code is 0.
:Status: *Unspecified*: whether a lossy or impossible conversion should be
         an error instead of a warning with status 0.
:Trace: ``src/al_lowlevel.cpp:680-741`` (``convertData``,
        ``setConvertedValue``), ``:1208-1218``.

.. _REQ-DATA-024:

REQ-DATA-024: Absolute field paths in reads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A ``fieldpath`` starting with ``/``, read from an operation or
            array of structures context, shall be resolved from the IDS
            root. IMAS-Core compares its segments with the chain of open
            AoS contexts and keeps the current index of each AoS level the
            target is under. A target under an AoS level that is not open
            shall be reported as missing (:ref:`REQ-DATA-021`).
:Nominal: With ``time_slice`` and then ``constraints/j_phi`` open, at
          indices ``(1, 2)``:

          - ``/time_slice/constraints/j_phi/reconstructed`` reads the same
            value as ``reconstructed``;
          - ``/time_slice/global_quantities/ip`` reads ``ip`` of
            ``time_slice[1]``;
          - ``/vacuum_toroidal_field/r0`` reads the root field.
:Limits: Segments are matched whole: ``time_slice_extra`` does not match
         ``time_slice``.
:Status: *Defect* on the baseline: HDF5 prepends the open context's path to
         the absolute path. Stored data then reads back as ``EMPTY_*``
         with status 0, and a root field cannot be read from the operation
         context (issue #65). Fixed for HDF5 reads by pending PR #103 (ADR
         0002). Writes, deletes and other backends are not covered by that
         fix.
:Trace: ``include/al_lowlevel.h:390-413``. ADR
        ``docs/adr/0002-absolute-field-path-resolution.md`` (PR #103);
        contract tests ``Hdf5AbsoluteFieldPaths.*``.

Deleting
--------

.. _REQ-DATA-030:

REQ-DATA-030: Delete path semantics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_delete_data(ctx, path)`` shall act on the IDS occurrence of
            the operation context ``ctx``:

            - A non-empty ``path`` deletes that node and its whole subtree.
              Paths are matched segment by segment, so deleting ``time``
              leaves ``time_slice`` in place. A path inside an AoS
              addresses that node in every element.
            - An empty ``path`` deletes the whole IDS occurrence.
            - Deleting a path that holds no data does nothing and succeeds.

            A backend that cannot delete at the requested granularity
            shall return an error instead of reporting success.
:Nominal: IMAS-Python's ``put()`` deletes every leaf and every AoS of the
          IDS in a ``WRITE_OP`` action before writing. ``delete_data()``
          does the same without writing.
:Limits: On HDF5, deleting all leaves one by one keeps the occurrence file
         and its master-file link, so ``al_get_occurrences`` still lists the
         occurrence (:ref:`REQ-QRY-001`). Only an empty path removes them.
:Errors: - ``ctx`` is not an operation context: ``LOWLEVEL_ERR`` "Wrong
           Context type stored".
         - Invalid handle: ``LOWLEVEL_ERR`` "Cannot find context N in
           store".
:Status: *Defect*. The current behavior per backend:

         .. list-table::
            :header-rows: 1

            * - Backend
              - Node
              - Subtree
              - Whole occurrence (``path == ""``)
            * - HDF5 (baseline)
              - deletes the whole occurrence
              - deletes the whole occurrence
              - yes, but leaves a dangling master-file link
            * - HDF5 (pending PR #101)
              - yes
              - yes
              - yes, and removes the link
            * - Memory
              - yes
              - yes for an AoS
              - does nothing, reports success
            * - MDSplus
              - yes
              - ``%TREE-W-NNF`` error
              - ``%TREE-W-NNF`` error
            * - ASCII, Flexbuffers
              - does nothing, reports success
              - does nothing, reports success
              - does nothing, reports success
            * - UDA
              - remote: refused; fetch mode: as the local backend
              - same
              - same

         Before PR #101, HDF5's leaf-by-leaf delete in IMAS-Python wiped
         the whole file at the first call. This hid a silent corruption
         downstream (issue #63).
:Trace: ``src/al_lowlevel.cpp:1242-1270``;
        ``src/hdf5/hdf5_backend.cpp:130-135`` (the path is not forwarded on
        the baseline); ``src/memory_backend.cpp:275-300``;
        ``src/ascii_backend.cpp:648-652``;
        ``src/flexbuffers_backend.cpp:387-389``. ADR
        ``docs/adr/0001-al-delete-data-path-semantics.md`` (PR #101).
        Contract tests ``Backends/DeleteMatrix.*``, ``Hdf5Delete.*``,
        ``DeleteKnownDefects.*``, ``test_mdsplus_delete.cpp``. IMAS-Python
        ``imas/backends/imas_core/db_entry_helpers.py:133-139``.

.. _REQ-DATA-031:

REQ-DATA-031: Delete access mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_delete_data`` is meant to be called in a ``WRITE_OP``
            operation context.
:Status: *Unspecified*: the core does not check the access mode, so a
         delete in a ``READ_OP`` context reaches the backend.
:Trace: ``src/al_lowlevel.cpp:1242-1253``.
