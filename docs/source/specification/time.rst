Time slices, interpolation and time ranges
==========================================

A *slice* operation reads or writes one time of an IDS. A *time range*
operation reads all the times in an interval, either as stored or resampled
on a new time vector. This page specifies which samples are selected and
how values are interpolated. How the operations are started, and the
validation of their arguments, are specified in :doc:`operations`. Which
backends support them is in :doc:`backends`.

The selection and interpolation rules live in ``src/data_interpolation.cpp``,
which HDF5 uses. MDSplus and Memory have their own code. Where they differ,
it is noted below.

Time model
----------

.. _REQ-TIME-001:

REQ-TIME-001: Timebases
^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A dynamic node is written with the path of its time vector (its
            *timebase*), and its last dimension follows that vector
            (:ref:`REQ-DATA-002`). A timed AoS follows its timebase one
            element per time. Slice and time range reads shall select
            samples by comparing the requested times with the values of the
            node's timebase, which is assumed to be in increasing order.
:Nominal: In a homogeneous IDS, every dynamic node uses ``/time``.
:Limits: IMAS-Core does not check that the timebase is increasing, or that
         its length matches the data. ``EMPTY_DOUBLE`` (``-9e40``) values
         in a timebase are skipped when looking for the upper bound of a
         time range.
:Status: Implemented. *Unspecified*: the result for a timebase that is not
         increasing.
:Trace: ``src/data_interpolation.cpp:13-56``; IMAS-Python
        ``imas/backends/imas_core/db_entry_helpers.py:49-51``.

Slice reads
-----------

.. _REQ-TIME-010:

REQ-TIME-010: Slice read result
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a slice ``READ_OP`` at time ``t``, IMAS-Core shall return:

            - **Dynamic node:** its value at ``t`` under the interpolation
              mode of the action, with the same rank as stored and the last
              (time) dimension of size 1.
            - **Static node:** its stored value, unchanged.
            - **Timed AoS:** a single element (:ref:`REQ-AOS-030`).
:Nominal: ``get_slice`` of ``core_profiles`` at ``t`` gives each profile at
          ``t``.
:Errors: - Slice read without an interpolation mode: ``CONTEXT_ERR``
           "Missing interpmode" (see :doc:`operations`).
         - Empty time vector for a dynamic node: ``BACKEND_ERR``
           "Unexpected time vector with size 0".
         - Invalid interpolation mode at the backend: ``BACKEND_ERR``
           "Interpolation mode not supported".
:Status: Implemented (HDF5, Memory, MDSplus). ASCII and Flexbuffers refuse
         slice operations.
:Trace: ``src/hdf5/hdf5_hs_selection_reader.cpp:128-144`` (time dimension
        set to 1); ``src/data_interpolation.cpp:93-176``. Contract tests
        ``Hdf5TimeDependent.SliceInterpolationModes``,
        ``Hdf5TimeDependent.SliceReadWithoutInterpModeIsRejected``,
        ``Backends/CapabilityMatrix.SliceReadPositiveOrRefused``.

.. _REQ-TIME-011:

REQ-TIME-011: CLOSEST_INTERP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: With ``CLOSEST_INTERP``, the selected sample shall be the one
            whose time is nearest to ``t``.
:Nominal: An exact match selects that sample.
:Limits: - ``t`` exactly halfway between two samples selects the **later**
           one.
         - ``t`` before the first sample selects the first; ``t`` after the
           last selects the last.
         - With a single sample, that sample is selected.
         - Because of floating-point error, a requested time computed as
           ``tmin + k*dt`` may not fall exactly halfway; IMAS-Python's tests
           use ``0.401`` instead of ``0.4`` for this reason.
:Status: Implemented. Memory applies the same tie rule
         (``time - t1 < t2 - time`` selects the earlier sample, otherwise
         the later).
:Trace: ``src/data_interpolation.cpp:110-160``;
        ``src/memory_backend.cpp:708-719``. IMAS-Python
        ``imas/test/test_get_sample.py:230-265,365``.

.. _REQ-TIME-012:

REQ-TIME-012: PREVIOUS_INTERP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: With ``PREVIOUS_INTERP``, the selected sample shall be the last
            one whose time is less than or equal to ``t``. A sample within
            machine epsilon (absolute, about ``2.2e-16``) of ``t`` counts as
            equal.
:Limits: - ``t`` after the last sample selects the last sample.
         - ``t`` before the first sample selects the **first** sample: data
           is returned, not an empty value.
         - The epsilon is absolute, so for times far from 1 the equality
           tolerance is in effect exact comparison.
:Status: Implemented. *Unspecified*: whether ``t`` before the first sample
         should give the first sample or an empty value.
:Trace: ``src/data_interpolation.cpp:162-169``.

.. _REQ-TIME-013:

REQ-TIME-013: LINEAR_INTERP
^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: With ``LINEAR_INTERP``, IMAS-Core shall take the two samples
            around ``t``, ``(t1, v1)`` and ``(t2, v2)``, and return
            ``v1 + (v2 - v1) * (t - t1) / (t2 - t1)``. The rules per type:

            - ``DOUBLE_DATA``: as above.
            - ``INTEGER_DATA``: as above, rounded to the nearest integer.
            - ``CHAR_DATA``: the common value when both samples are equal,
              otherwise the empty string.
:Limits: - ``t`` outside ``[t1, t2]`` (before the first or after the last
           sample) gives the nearest sample's value: constant
           extrapolation, never linear extrapolation.
         - With a single sample, or ``t1 == t2``, the sample's value is
           returned.
         - HDF5: when the neighbouring sample cannot be read, or the node's
           shape differs between the two samples, the node is reported as
           missing (:ref:`REQ-DATA-021`). A ``WARNING: Linear interpolation
           not possible ...`` line is printed on standard output, unless the
           environment variable ``IMAS_AL_DISABLE_INTERPOLATION_WARNING`` is
           set.
:Status: - *Defect*: ``COMPLEX_DATA`` is not handled by
           ``DataInterpolation::interpolate``. The result is left unset.
         - *Defect*: for ``CHAR_DATA``, ``interpolate`` never assigns its
           result pointer. Slice reads work only because HDF5 passes the
           output buffer as the lower sample, which is updated in place.
           Resampled time range reads use an uninitialized pointer (see
           :ref:`REQ-TIME-033`).
:Trace: ``src/data_interpolation.cpp:179-290``;
        ``src/hdf5/hdf5_reader.cpp:898-955,1185-1207``.

Slice writes
------------

.. _REQ-TIME-020:

REQ-TIME-020: Appending a slice
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a slice ``WRITE_OP`` action started with ``UNDEFINED_TIME``,
            each dynamic node written shall get one more sample appended
            along its time dimension, and each timed AoS one more element.
            The data passed has the time dimension of size 1. Successive
            slice writes shall accumulate.
:Nominal: IMAS-Python's ``put_slice`` writes only dynamic nodes, with
          ``UNDEFINED_TIME`` and ``UNDEFINED_INTERP``. When the IDS
          occurrence does not exist yet (``homogeneous_time`` reads as
          empty), it does a full ``put`` instead.
:Limits: Writing static nodes in a slice action is not specified. HLIs
         shall not do it.
:Status: *Defect* (pinned by a contract test, not yet analysed): on HDF5,
         only the last of several slices appended with ``UNDEFINED_TIME``
         persists. *Defect* (crash): on MDSplus, a scalar write inside a
         slice action dereferences a null size pointer (see
         :doc:`backends`).
:Trace: ``include/al_lowlevel.h:300-316``. Contract tests
        ``Hdf5SliceAppend.DISABLED_AppendedSlicesAllPersist``,
        ``Backends/CapabilityMatrix.SliceWriteBeginPositiveOrRefused``.
        IMAS-Python ``imas/backends/imas_core/db_entry_al.py:306-330``,
        ``db_entry_helpers.py:194-195``.

Time range reads
----------------

.. _REQ-TIME-030:

REQ-TIME-030: Time range without resampling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a time range ``READ_OP`` with an empty ``dtime``, each
            dynamic node shall be returned with the samples whose time
            ``t_i`` satisfies ``tmin <= t_i <= tmax`` (both bounds
            inclusive), and timed AoS with the matching elements. Static
            nodes are returned whole, and the interpolation mode is not
            used.
:Nominal: ``get_sample(ids, tmin, tmax)`` in IMAS-Python. A heterogeneous
          IDS stays heterogeneous.
:Limits: - ``tmin == tmax`` returns the sample at exactly that time, if
           there is one.
         - No sample in the interval gives an empty result for that node.
:Errors: ``tmin > tmax`` shall fail with ``CONTEXT_ERR``.
:Status: - *Defect*: the C API does not check ``tmin > tmax``. The Python
           binding raises ``ValueError``.
         - *Defect* (out-of-bounds read): when samples exist at or after
           ``tmin`` but none at or before ``tmax`` (for example
           ``tmin <= tmax < t_0``), the reverse search loop uses an
           unsigned index and runs past index 0.
:Trace: ``src/data_interpolation.cpp:17-55``;
        ``python/imas_core/_al_lowlevel.pyx:456-460``. IMAS-Python
        ``imas/test/test_get_sample.py:108-160``. Contract test
        ``Hdf5TimeDependent.TimeRangeReadWithoutResampling``.

.. _REQ-TIME-031:

REQ-TIME-031: Resampling on a uniform time step
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a time range ``READ_OP`` where ``dtime`` has one value
            ``dt``, every dynamic node and timed AoS shall be resampled at
            the times ``tmin + k*dt``, for ``k = 0, 1, ...``, up to and
            including the last time that is not greater than ``tmax``. Each
            value is computed with the action's interpolation mode
            (:ref:`REQ-TIME-011` to :ref:`REQ-TIME-013`). The result is a
            homogeneous IDS: HDF5 returns ``ids_properties/homogeneous_time``
            as 1.
:Nominal: ``get_sample("core_profiles", 0.3, 0.401, 0.02, LINEAR_INTERP)``
          gives 6 times.
:Limits: The number of times is ``round((tmax - tmin)/dt)``, plus 1 when
         the last computed time is not greater than ``tmax``. This depends
         on floating-point rounding near ``tmax``.
:Errors: - ``dtime`` given without an interpolation mode: ``CONTEXT_ERR``
           "Missing interpmode (dtime.size() != 0)" (see :doc:`operations`).
         - ``dt <= 0`` shall fail with ``CONTEXT_ERR``.
:Status: *Defect*: ``dt <= 0`` is not checked. The division gives an
         infinite or negative number of samples (undefined behavior).
:Trace: ``src/data_interpolation.cpp:57-91,294-342``;
        ``src/hdf5/hdf5_reader.cpp:874-882``. IMAS-Python
        ``imas/db_entry.py:460-480,541``; ``imas/test/test_get_sample.py``.

.. _REQ-TIME-032:

REQ-TIME-032: Resampling on an explicit time vector
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: In a time range ``READ_OP`` where ``dtime`` has more than one
            value, every dynamic node and timed AoS shall be resampled at
            exactly the times in ``dtime``. ``tmin`` and ``tmax`` are
            ignored. The rest is as in :ref:`REQ-TIME-031`.
:Limits: ``dtime`` shall be increasing.
:Status: Implemented. *Unspecified*: the result for a ``dtime`` that is not
         increasing.
:Trace: ``src/data_interpolation.cpp:57-91,412-425``. IMAS-Python
        ``imas/test/test_get_sample.py`` (explicit timebase).

.. _REQ-TIME-033:

REQ-TIME-033: Data types in resampling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: Resampling shall support ``DOUBLE_DATA``, ``INTEGER_DATA`` and
            ``CHAR_DATA`` with the rules of :ref:`REQ-TIME-013`.
            ``COMPLEX_DATA`` shall be refused with ``BACKEND_ERR`` rather
            than returned undefined.
:Status: *Defect* (memory corruption): each resampled slice is copied with
         ``time_slice_shape * sizeof(double)`` bytes, whatever the type. For
         ``INTEGER_DATA`` and ``CHAR_DATA`` this overflows the
         heap buffer. Also:

         - The ``CHAR_DATA`` ``LINEAR_INTERP`` result pointer is
           uninitialized (:ref:`REQ-TIME-013`).
         - ``COMPLEX_DATA`` is not handled at all.
         - With ``LINEAR_INTERP``, the upper sample is read at
           ``offset + 1`` even when ``offset`` is the last sample.
:Trace: ``src/data_interpolation.cpp:338-609`` (copy at ``:552``).

.. _REQ-TIME-034:

REQ-TIME-034: Time range support
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: A backend that cannot perform time range operations shall
            refuse them when the action starts, with ``LOWLEVEL_ERR``
            "Selected backend does not support time range operations."
:Status: - Implemented: HDF5 supports them. Memory, ASCII, Flexbuffers and
           MDSplus refuse them.
         - *Defect* on UDA: the capability is granted for servers above
           version 1.4.0, but the read then fails with "unknown interp
           mode", because the time range constructor of
           ``OperationContext`` leaves the base interpolation mode unset
           (see :doc:`operations`).
:Trace: ``src/al_lowlevel.cpp:1052-1055``. Contract test
        ``Backends/CapabilityMatrix.TimeRangeReadPositiveOrRefused``.
