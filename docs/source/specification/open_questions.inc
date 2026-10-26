URIs and data entries
   - :ref:`REQ-URI-002`: should a URI scheme other than ``imas`` be
     refused?
   - :ref:`REQ-URI-005`: when a query key is repeated, should IMAS-Core
     refuse the URI, keep the last value, or join the values as it does
     today?
   - :ref:`REQ-DE-023`: what may be done with a data entry handle after
     ``al_close_pulse`` and before ``al_end_action``?
   - :ref:`REQ-DE-024`: is ``al_end_action`` on a data entry that was not
     closed allowed, and must it flush the data?
   - :ref:`REQ-DE-030`: is opening the same data entry through two handles
     supported, including for concurrent writes?

Contexts and operations
   - :ref:`REQ-CTX-004`: should a stale handle that was reused for a new
     context be detected, for example with a generation counter?
   - :ref:`REQ-CTX-012`: when a parent context is ended while it still has
     open children, should IMAS-Core refuse with ``CONTEXT_ERR`` or end the
     children too?
   - :ref:`REQ-CTX-013`: may several operations be open on one data entry
     at the same time?
   - :ref:`REQ-OP-002`: should ``REPLACE_OP`` be specified or removed?
   - :ref:`REQ-OP-003`: should the IDS name and occurrence be validated?
   - :ref:`REQ-ERR-005`: should ``al_build_uri_from_legacy_parameters``
     report argument errors as ``CONTEXT_ERR`` instead of
     ``LOWLEVEL_ERR``? Callers would see the change.

Data, arrays of structures and time
   - :ref:`REQ-DATA-001`: should the core refuse ``dim > MAXDIM`` (7)?
   - :ref:`REQ-DATA-010`: should a write in a ``READ_OP`` operation be
     refused?
   - :ref:`REQ-DATA-023`: should a lossy type conversion on read, or a
     conversion from complex data, be an error instead of a warning with
     status 0?
   - :ref:`REQ-DATA-031`: should ``al_delete_data`` require a ``WRITE_OP``
     operation?
   - :ref:`REQ-AOS-010`: should moving the AoS cursor outside
     ``[0, size)`` be refused?
   - :ref:`REQ-TIME-001`, :ref:`REQ-TIME-032`: what should happen with a
     timebase or a ``dtime`` vector that is not increasing?
   - :ref:`REQ-TIME-012`: should ``PREVIOUS_INTERP`` before the first
     sample return the first sample or no data?
   - :ref:`REQ-QRY-001`: should ``al_get_occurrences`` check that it is
     given a data entry context?
   - :ref:`REQ-QRY-022`: should ``TIMERANGE_OP`` and
     ``FLEXBUFFERS_BACKEND`` get names in ``const2str``?

Backends, plugins and non-functional
   - :ref:`REQ-BE-004`: should invalid values of known URI options be
     refused? HDF5 silently uses the defaults, UDA refuses them.
   - :ref:`REQ-BE-031`: may the MDSplus 1.1 backend write into a 1.0 entry
     (one without version nodes)? Today the write is refused.
   - :ref:`REQ-BE-073`: should the Memory backend implement
     ``al_get_occurrences`` and ``al_list_filled_paths``?
   - :ref:`REQ-BE-100`: should ``NO_BACKEND`` be reachable from a URI, or
     removed?
   - :ref:`REQ-PLG-022`: should a failed readback setup undo the bindings
     it already made?
   - :ref:`REQ-NFR-001`: which thread-safety guarantee, if any, does
     IMAS-Core give?
