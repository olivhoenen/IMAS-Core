Behavior specification
======================

This section is the Software Requirements Document (SRD) of IMAS-Core. It
specifies the externally observable behavior of the low-level Access Layer:
its C/C++ API (``include/al_lowlevel.h``), the Python binding
(``imas_core``) and the storage backends. For each requirement it gives the
nominal cases, the limit cases and the error cases.

It complements the :doc:`../user_guide/index`, which explains how to use
IMAS-Core, and the :doc:`../developers/index`, which explains how it is built
and extended. Where a guide already describes a behavior, this document links
to it instead of repeating it.

Scope and baseline
------------------

- **In scope:** all functions declared in ``include/al_lowlevel.h``, the
  constants of ``al_defs.h`` and ``al_const.h``, the exception classes, the
  URI handling, the backends built from ``src/`` (HDF5, MDSplus, UDA, Memory,
  ASCII, Flexbuffers, NoBackend), the plugin framework and the
  ``imas_core`` Python package.
- **Out of scope:** the high-level interfaces built on top of IMAS-Core
  (IMAS-Python, and the Fortran, C++, Java and MATLAB HLIs), the content of
  the IMAS Data Dictionary, and the UDA server and its plugins. The behavior
  that IMAS-Python relies on is listed in :doc:`consumers`.
- **Baseline:** branch ``develop`` at release ``5.7.2`` plus 17 commits
  (``e4b47af5``). Behavior that only exists on an open pull request is
  marked *pending PR #N*. When a pull request is merged, its requirements
  are updated.

The behavior described here comes from the code, the existing documentation,
the contract test suite (pull requests #95 to #99, ``tests/contract/``), the
architecture decision records of pull requests #101 and #103
(``docs/adr/``) and the way IMAS-Python uses IMAS-Core.

Conventions
-----------

Requirement identifiers
~~~~~~~~~~~~~~~~~~~~~~~

Each requirement has a stable identifier ``REQ-<AREA>-<NNN>``. The areas are:

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Area
     - Topic
   * - ``URI``
     - URI syntax, backend selection and legacy parameters (:doc:`uri`)
   * - ``DE``
     - Opening and closing data entries, version compatibility
       (:doc:`data_entry`)
   * - ``CTX``
     - Context handles and their lifecycle (:doc:`contexts`)
   * - ``OP``
     - Global, slice and time range operations (:doc:`operations`)
   * - ``DATA``
     - Reading, writing and deleting data (:doc:`data_access`)
   * - ``AOS``
     - Arrays of structures (:doc:`aos`)
   * - ``TIME``
     - Time slices, interpolation and time ranges (:doc:`time`)
   * - ``QRY``
     - Queries and introspection (:doc:`queries`)
   * - ``BE``
     - Backend-specific behavior (:doc:`backends`)
   * - ``ERR``
     - Status codes, exceptions and messages (:doc:`errors`)
   * - ``PLG``
     - Plugins (:doc:`plugins`)
   * - ``PY``
     - Python binding (:doc:`python_binding`)
   * - ``NFR``
     - Non-functional requirements (:doc:`nonfunctional`)

Identifiers are never reused. A requirement that is dropped stays in the
document with the status *Withdrawn*.

Requirement layout
~~~~~~~~~~~~~~~~~~

The key words *shall*, *shall not*, *should* and *may* are used as in
`RFC 2119 <https://www.rfc-editor.org/rfc/rfc2119>`_. Each requirement has
this layout:

.. code-block:: rst

   .. _REQ-AREA-NNN:

   REQ-AREA-NNN: Short title
   ^^^^^^^^^^^^^^^^^^^^^^^^^

   :Statement: What IMAS-Core shall do.
   :Nominal: Normal use and its expected result.
   :Limits: Boundary values and their expected result.
   :Errors: Invalid use, with the expected status code or exception and
            message.
   :Status: Implemented | Defect | Divergence | Unspecified
   :Trace: Code (``src/file.cpp:line``), tests, pull requests, consumers.

Fields that do not apply are left out. Line numbers refer to the baseline and
may drift; the function names stay valid.

Status values
~~~~~~~~~~~~~

Implemented
   The baseline behaves as specified.

Defect
   The requirement states the **intended** behavior and the baseline does
   not meet it. The *Status* field describes the current behavior and links
   to the evidence: a ``DISABLED_`` contract test with its tripwire, an
   issue or a fixing pull request. All defects are collected in
   :doc:`defects`.

Divergence
   Backends behave differently and the difference is accepted, usually
   because of their storage model. The *Status* field explains it.

Unspecified
   The intended behavior is not settled. The *Status* field describes the
   current behavior, and the question is listed in
   :ref:`spec-open-questions`.

Glossary
--------

Data entry
   A storage location that holds IDSs, identified by a URI. Older
   documents call it a *pulse file*; the API still uses ``pulse`` in
   function and constant names.

IDS
   Interface Data Structure, a tree of nodes defined by the |DD|. IMAS-Core
   does not interpret the Data Dictionary: it stores the paths and values
   it is given.

Occurrence
   One of several instances of the same IDS in a data entry, numbered from
   0. Occurrence ``N`` of IDS ``x`` is addressed as ``x/N``; ``x`` alone and
   ``x/0`` are the same occurrence.

Context, handle
   An integer that identifies an object kept by IMAS-Core: a data entry
   context, an operation context or an array of structures context. Handle
   ``0`` stands for "no context".

Operation
   A read or write of one IDS occurrence, between ``al_begin_*_action``
   and ``al_end_action``. It is global (the whole IDS), a slice (one time)
   or a time range.

Array of structures (AoS)
   A node that holds a list of structures. It is *timed* when it has a
   timebase, i.e. its elements follow a time vector.

Timebase
   The path of the time vector that a dynamic node or a timed AoS follows,
   passed with each read and write. An empty timebase means the data is
   static.

``homogeneous_time``
   The node ``ids_properties/homogeneous_time`` of an IDS. ``1`` means all
   dynamic nodes use the root ``/time`` vector, ``0`` means each node has
   its own timebase, ``2`` means the IDS has no time dependence. High-level
   interfaces read it to find out whether an IDS exists.

EMPTY sentinel
   The value that stands for "no data" for a scalar: ``EMPTY_INT``
   (``-999999999``), ``EMPTY_DOUBLE`` (``-9.0E40``), ``EMPTY_COMPLEX``
   (``-9.0E40`` for both parts) and ``EMPTY_CHAR`` (``'\0'``).

HLI
   High-level interface, i.e. a language binding built on top of IMAS-Core,
   such as IMAS-Python.

.. _spec-open-questions:

Open questions
--------------

The requirements marked *Unspecified* raise these questions for the
maintainers. Each answer turns the requirement into *Implemented* or
*Defect*.

.. include:: open_questions.inc

Contents
--------

.. toctree::
   :maxdepth: 2

   architecture
   uri
   data_entry
   contexts
   operations
   data_access
   aos
   time
   queries
   backends
   errors
   plugins
   python_binding
   nonfunctional
   consumers
   defects
   traceability
