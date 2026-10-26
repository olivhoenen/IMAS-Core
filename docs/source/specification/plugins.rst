Plugins
=======

This page specifies the plugin framework: how a plugin is registered, bound
to IDS paths, called during reads and writes, and how its provenance is
stored. The design is described in :doc:`../developers/plugins_architecture`
and the C++ interface in :doc:`../developers/plugins_framework`.

All plugin state (registered plugins and bindings) belongs to the process.
It is shared by all data entries and is not protected against concurrent
use from several threads.

Framework and registration
--------------------------

.. _REQ-PLG-001:

REQ-PLG-001: Plugin framework switch
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The plugin framework shall be enabled only when the environment
            variable ``IMAS_AL_ENABLE_PLUGINS`` equals ``TRUE`` exactly.
            When it is disabled, the plugin management calls shall fail and
            reads and writes shall ignore plugins.
:Nominal: With ``IMAS_AL_ENABLE_PLUGINS=TRUE``, ``al_register_plugin``
          works.
:Limits: The value is case-sensitive: ``true`` or ``1`` leave the framework
         disabled. The variable is read on every call, so changing it while
         the process runs takes effect at once. An empty value written to
         a field with a bound plugin is passed to the plugin. Without a
         bound plugin, the core drops it (see :doc:`data_access`).
:Errors: With the framework disabled, ``al_register_plugin``,
         ``al_unregister_plugin``, ``al_is_plugin_registered`` and
         ``al_bind_plugin`` return ``LOWLEVEL_ERR`` "Plugins feature is
         disabled. Set the global variable 'IMAS_AL_ENABLE_PLUGINS' to
         'TRUE' to enable this feature."
:Status: Implemented. ``al_unbind_plugin`` and the
         ``al_setvalue_*_parameter_plugin`` calls do not check the switch.
:Trace: ``src/al_lowlevel.cpp:108-119``. Contract tests
        ``PluginTest.{RegisterWithFrameworkDisabledReturnsError,
        IsPluginRegisteredWithFrameworkDisabledReturnsError}``.
        :doc:`../user_guide/configuration`.

.. _REQ-PLG-002:

REQ-PLG-002: Registering a plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_register_plugin(name)`` shall load the shared library
            ``$IMAS_AL_PLUGINS/<name>_plugin.so`` and create one plugin
            instance with its ``create`` function. When registration fails,
            it shall return an error and leave the plugin unregistered.
:Nominal: After registration, ``al_is_plugin_registered`` returns true.
:Errors: All errors below return ``LOWLEVEL_ERR``:

         - ``IMAS_AL_PLUGINS`` not set: "IMAS_AL_PLUGINS environment
           variable not defined".
         - The name is already registered: "Plugin <name> already
           registered in the plugins store."
         - The library file is missing: "Plugin shared library <file> not
           found".
         - The library has no ``create`` or ``destroy`` symbol: "Cannot load
           symbol create:..." or "Cannot load symbol destroy:...".
:Status: Defect. When the library exists but cannot be loaded, the failure
         is only checked by an ``assert``. A release build then fails later
         with the misleading "Cannot load symbol create" message, and a
         debug build aborts. After any failure past the file check, the
         name stays in the registry, so a later
         ``al_is_plugin_registered`` returns true and a retry fails with
         "already registered".
:Trace: ``src/al_lowlevel.cpp:323-380``. Contract tests
        ``PluginTest.{RegisterMakesPluginRegistered, RegisterTwiceReturnsError}``,
        ``PluginTest.DISABLED_RegisterUnloadableSharedLibReportsDlopenFailure``.
        The variable ``IMAS_AL_PLUGINS`` is not listed in
        :doc:`../user_guide/configuration`.

.. _REQ-PLG-003:

REQ-PLG-003: Unregistering a plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_unregister_plugin(name)`` shall remove all bindings of the
            plugin, destroy its instance and remove it from the registry,
            whether or not it is bound.
:Nominal: A bound plugin is destroyed and all its bindings are removed.
:Errors: An unregistered name returns ``LOWLEVEL_ERR`` "Plugin <name> not
         registered in the plugins store."
:Status: Defect. A registered plugin that was never bound is neither
         destroyed nor removed, and the call returns status 0.
:Trace: ``src/al_lowlevel.cpp:382-431``. Contract tests
        ``PluginTest.{UnregisterNeverRegisteredNameReturnsError,
        UnregisterBoundPluginDestroysIt}``,
        ``PluginTest.DISABLED_UnregisterNeverBoundPluginDestroysIt``.

.. _REQ-PLG-004:

REQ-PLG-004: Querying registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_is_plugin_registered(name, &flag)`` shall set ``flag`` to
            true when the name is in the registry and false otherwise.
:Errors: ``LOWLEVEL_ERR`` when the framework is disabled (REQ-PLG-001).
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:318-321, 1929-1948``. Contract tests
        ``PluginTest.IsPluginRegisteredIsFalseForNeverRegisteredName``.

Bindings
--------

.. _REQ-PLG-010:

REQ-PLG-010: Binding a plugin to a path
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_bind_plugin(path, name)`` shall attach a registered plugin
            to an IDS path. The path syntax is
            ``[#]<ids>[:<occurrence>][/<node path>]``, where array indices
            written as ``(:)`` or ``(,)`` are ignored, or
            ``[#]<ids>[:<occurrence>]/*`` for all nodes of the IDS. A
            missing occurrence means occurrence 0. Several plugins may be
            bound to the same path; they are applied in binding order.
:Nominal: ``al_bind_plugin("equilibrium/time", "p")`` binds ``p`` to node
          ``time`` of occurrence 0. ``al_bind_plugin("equilibrium:1/*",
          "p")`` binds it to every node of occurrence 1.
:Limits: Binding all nodes prints a line to standard output ("all IDS nodes
         are bound, ...").
:Errors: All errors below return ``LOWLEVEL_ERR``:

         - The plugin is not registered: "Plugin <name> is not registered.
           Plugins need to be registered using al_register_plugin(name)
           before to be bound."
         - The same plugin is already bound to the path: "Plugin <name> is
           already bound to path: <path>."
         - The path does not match the syntax: "bindPlugin: bad format: ..."
           or "bindPlugin!!!: bad format: ...".
:Status: Implemented.
:Trace: ``src/al_lowlevel.cpp:191-283``. Contract tests
        ``PluginTest.{BindRegisteredPluginSucceeds,
        BindUnregisteredPluginReturnsError, DoubleBindSamePathReturnsError}``.

.. _REQ-PLG-011:

REQ-PLG-011: Unbinding a plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_unbind_plugin(path, name)`` shall accept the same path
            syntax as ``al_bind_plugin`` and remove that binding. Unbinding
            a path with no binding shall succeed and change nothing.
:Nominal: Binding then unbinding ``equilibrium/time`` leaves no binding.
:Status: Defect. ``al_unbind_plugin`` looks up the path string as given,
         but bindings are stored under a normalized key such as
         ``equilibrium:0/time``. Unbinding with the path given to
         ``al_bind_plugin`` therefore prints "No plugin bound to field
         path: ..." and leaves the binding in place, with status 0. Only the
         normalized form works.
:Trace: ``src/al_lowlevel.cpp:297-316``. Contract test
        ``PluginTest.UnbindNeverBoundPathIsSilentNoOp``; the mismatch is
        described in ``tests/contract/test_plugins.cpp`` (PR #95 branch).

.. _REQ-PLG-012:

REQ-PLG-012: Plugins replace the backend for bound paths
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: When the framework is enabled and plugins are bound to the full
            path of a field (data object, occurrence and node path), the
            following shall happen:

            - ``al_write_data`` and ``al_read_data`` on that field shall call
              each bound plugin, in binding order, instead of the backend.
            - ``al_begin_*_action``, ``al_begin_arraystruct_action`` and
              ``al_end_action`` shall also notify the plugins bound to
              the data object.
            - A plugin may call the ``al_plugin_*`` functions to reach the
              backend directly.
:Nominal: A plugin bound to ``magnetics/probe`` intercepts the write. The
          backend receives nothing unless the plugin calls
          ``al_plugin_write_data``.
:Limits: The ``<buffer>`` field of the Flexbuffers backend is never sent to
         plugins. A binding to ``<ids>:<occurrence>/*`` applies to every
         field of that occurrence.
:Errors: Errors raised by a plugin as ``ALPluginException`` return
         ``LOWLEVEL_ERR``.
:Status: Defect. The fallback lookup for ``<ids>:*/*``, meant to cover all
         occurrences, cuts the last character of the IDS name and can never
         match. Also, no binding syntax produces such a key. The data
         object lookup used by the begin actions matches keys by prefix.
         Plugins bound to occurrence 10 are therefore also notified for
         occurrence 1. It also stops at the first matching key.
:Trace: ``src/al_lowlevel.cpp:121-189, 1420-1450, 1677-1745``. Contract
        tests ``PluginTest.BoundPluginInterceptsWriteInsteadOfBackend``,
        ``PluginTest.BoundPluginInterceptsReadInsteadOfBackend``,
        ``PluginReentry.{WriteDataReentersTheBackendDirectly,
        ReadDataAlsoReentersTheBackendDirectly}``.

.. _REQ-PLG-013:

REQ-PLG-013: Low-level re-entry for plugins
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: The ``al_plugin_*`` functions (``begin_global_action``,
            ``begin_slice_action``, ``begin_timerange_action``,
            ``begin_arraystruct_action``, ``read_data``, ``write_data``,
            ``end_action``) shall behave like the matching ``al_*``
            functions without plugin dispatch.
:Nominal: Writing with ``al_plugin_write_data`` and reading back with
          ``al_read_data`` gives the same data as ``al_write_data``.
:Status: Implemented for the functions covered by the contract suite.
         ``al_plugin_begin_timerange_action`` is not covered: the contract
         ledger reports a separate upstream issue for it. In the baseline,
         its declaration and definition have the same signature. Like
         ``al_begin_timerange_action``, it never sets the interpolation
         mode of the operation context (see :doc:`time`).
:Trace: ``include/al_lowlevel.h:294-453``, ``src/al_lowlevel.cpp:937-1330``.
        Contract tests ``PluginReentry.*``.

Parameters and provenance
-------------------------

.. _REQ-PLG-020:

REQ-PLG-020: Setting plugin parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_setvalue_parameter_plugin``,
            ``al_setvalue_int_scalar_parameter_plugin`` and
            ``al_setvalue_double_scalar_parameter_plugin`` shall pass the
            name and value to the plugin's ``setParameter``. A plugin name
            that is not registered shall return an error status.
:Nominal: On a registered plugin, ``setParameter`` receives the exact value.
:Limits: The rank given to ``al_setvalue_parameter_plugin`` is not limited
         by ``MAXDIM``.
:Status: Defect (crash). For an unregistered name, all three calls create an
         empty registry entry and call through a null pointer, so the
         process crashes.
:Trace: ``src/al_lowlevel.cpp:453-457, 1803-1883``. Contract tests
        ``PluginTest.SetValue{Generic,IntScalar,DoubleScalar}OnRegisteredPluginReachesPlugin``,
        ``PluginTest.SetValueGenericAcceptsDimAboveMaxdim``,
        ``KnownDefects.DISABLED_SetValue{Generic,IntScalar,DoubleScalar}UnregisteredPluginReturnsError``.

.. _REQ-PLG-021:

REQ-PLG-021: Storing plugin provenance
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_write_plugins_metadata(ctx)``, called at the end of a
            write, shall store the provenance of every plugin bound for
            writing to the IDS of ``ctx``: name, version, commit,
            repository, parameters and node path. It also stores the
            readback plugins the plugin declares. The data goes under
            ``ids_properties/plugins/node``.
:Nominal: After a write with one bound plugin, the stored ``path`` and
          ``put_operation`` fields match the plugin's provenance, and
          ``readback`` is empty.
:Errors: A plugin application index out of range, or used twice for the
         same node, gives "Plugin index (N) is out of bounds ..." or
         "Plugin index (N) is already defined ...". The error is raised as
         ``ALBackendException``, and since the function has no handler for
         that class, it returns ``UNKNOWN_ERR``.
:Status: Implemented.
:Trace: ``src/access_layer_plugin_manager.cpp:359-489``,
        ``src/al_lowlevel.cpp:1992-2012``. Contract test
        ``PluginTest.WritePluginsMetadataStoresBoundPluginProvenance``.

.. _REQ-PLG-022:

REQ-PLG-022: Readback plugins
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

:Statement: ``al_bind_readback_plugins(ctx)``, called before a read, shall
            read the plugin metadata stored in the IDS, then register and
            bind each readback plugin it names to its node path.
            ``al_unbind_readback_plugins(ctx)``, called after the read,
            shall remove those bindings and unregister those plugins.
:Nominal: An IDS written with a plugin that declares a readback plugin is
          read through that readback plugin.
:Errors: The errors below are raised as ``ALBackendException`` and return
         ``UNKNOWN_ERR`` (see :doc:`errors`):

         - The stored plugin is declared ``PUT_ONLY`` for the path:
           "Readback plugin <name> is declared PUT_ONLY for path=<path>."
         - The installed version differs from the stored one: "Readback
           plugin <name> has not the expected version, found=X,
           expected=Y."
:Status: Unspecified. After one of these errors, the plugin is left bound.
         It is not settled whether a failed readback setup should undo its
         bindings.
:Trace: ``src/access_layer_plugin_manager.cpp:52-276``. Contract test
        ``ReadbackPlugins.BindReadbackPluginsAutoRegistersAndBindsFromStoredMetadata``.
