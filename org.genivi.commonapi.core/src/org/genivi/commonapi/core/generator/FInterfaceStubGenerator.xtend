/* Copyright (C) 2013, 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.HashMap
import java.util.HashSet
import java.util.LinkedHashMap
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants

class FInterfaceStubGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

    var HashMap<String, Integer> counterMap;
    var HashMap<FMethod, LinkedHashMap<String, Boolean>> methodrepliesMap;

    def generateStub(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        if(FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true").equals("true")) {
            fileSystemAccess.generateFile(fInterface.stubHeaderPath, PreferenceConstants.P_OUTPUT_STUBS, fInterface.generateStubHeader(deploymentAccessor, modelid))
            // should skeleton code be generated ?
            if(FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_SKELETON, "false").equals("true"))
            {
                fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.generateStubDefaultHeader(deploymentAccessor, modelid))
                fileSystemAccess.generateFile(fInterface.stubDefaultSourcePath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.generateStubDefaultSource(deploymentAccessor, modelid))
            }
        }
        else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fInterface.stubHeaderPath, PreferenceConstants.P_OUTPUT_STUBS, PreferenceConstants::NO_CODE)
            // should skeleton code be generated ?
            if(FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_SKELETON, "false").equals("true"))
            {
                fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants::NO_CODE)
                fileSystemAccess.generateFile(fInterface.stubDefaultSourcePath, PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants::NO_CODE)
            }
        }
    }

    def private generateStubHeader(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_STUB_HPP_
        #define «fInterface.defineName»_STUB_HPP_

        #include <functional>
        #include <sstream>

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»

        «fInterface.generateInheritanceIncludes(generatedHeaders, libraryHeaders)»
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders, true)»
        «fInterface.generateSelectiveBroadcastStubIncludes(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #include <«fInterface.headerPath»>

        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #include <CommonAPI/Stub.hpp>

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        /**
         * Receives messages from remote and handles all dispatching of deserialized calls
         * to a stub for the service «fInterface.elementName». Also provides means to send broadcasts
         * and attribute-changed-notifications of observable attributes as defined by this service.
         * An application developer should not need to bother with this class.
         */
        class «fInterface.stubAdapterClassName»
            : public virtual CommonAPI::StubAdapter,
              public virtual «fInterface.elementName»«IF fInterface.base != null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubAdapter«ENDIF» {
         public:
            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    ///Notifies all remote listeners about a change of value of the attribute «attribute.elementName».
                    virtual void «attribute.stubAdapterClassFireChangedMethodName»(const «attribute.getTypeName(fInterface, true)»& «attribute.elementName») = 0;
                «ENDIF»
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «IF broadcast.selective»
                    /**
                     * Sends a selective broadcast event for «broadcast.elementName». Should not be called directly.
                     * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                     */
                    virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateFireSelectiveSignatur(broadcast, fInterface)») = 0;
                    virtual void «broadcast.stubAdapterClassSendSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)») = 0;
                    virtual void «broadcast.subscribeSelectiveMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, bool& success) = 0;
                    virtual void «broadcast.unsubscribeSelectiveMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId) = 0;
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»() = 0;
                «ELSE»
                    «IF (!broadcast.isErrorType(deploymentAccessor))»
                        /**
                        * Sends a broadcast event for «broadcast.elementName». Should not be called directly.
                        * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                        */
                        virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») = 0;
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual «managed.stubRegisterManagedMethod» = 0;
                virtual bool «managed.stubDeregisterManagedName»(const std::string&) = 0;
                virtual std::set<std::string>& «managed.stubManagedSetGetterName»() = 0;
            «ENDFOR»

            virtual void deactivateManagedInstances() = 0;
        protected:
            /**
             * Defines properties for storing the ClientIds of clients / proxies that have
             * subscribed to the selective broadcasts
             */
            «FOR broadcast : fInterface.broadcasts»
                «IF broadcast.selective»
                    std::shared_ptr<CommonAPI::ClientIdList> «broadcast.stubAdapterClassSubscriberListPropertyName»;
                «ENDIF»
            «ENDFOR»
        };

        /**
         * Defines the necessary callbacks to handle remote set events related to the attributes
         * defined in the IDL description for «fInterface.elementName».
         * For each attribute two callbacks are defined:
         * - a verification callback that allows to verify the requested value and to prevent setting
         *   e.g. an invalid value ("onRemoteSet<AttributeName>").
         * - an action callback to do local work after the attribute value has been changed
         *   ("onRemote<AttributeName>Changed").
         *
         * This class and the one below are the ones an application developer needs to have
         * a look at if he wants to implement a service.
         */
        class «fInterface.stubRemoteEventClassName»
        «IF fInterface.base != null»
            : public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubRemoteEvent
        «ENDIF»
        {
        public:
            virtual ~«fInterface.stubRemoteEventClassName»() { }

            «FOR attribute : fInterface.attributes»
                «IF !attribute.readonly»
                    /// Verification callback for remote set requests on the attribute «attribute.elementName»
                    virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «attribute.getTypeName(fInterface, true)» _value) = 0;
                    /// Action callback for remote set requests on the attribute «attribute.elementName»
                    virtual void «attribute.stubRemoteEventClassChangedMethodName»() = 0;
                «ENDIF»
            «ENDFOR»
        };

        /**
         * Defines the interface that must be implemented by any class that should provide
         * the service «fInterface.elementName» to remote clients.
         * This class and the one above are the ones an application developer needs to have
         * a look at if he wants to implement a service.
         */
        class «fInterface.stubClassName»
            : public virtual «fInterface.stubCommonAPIClassName»«IF fInterface.base != null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»Stub«ENDIF»
        {
        public:
        «{counterMap = new HashMap<String, Integer>();""}»
        «{methodrepliesMap = new HashMap<FMethod, LinkedHashMap<String, Boolean>>();""}»
        «FOR method: fInterface.methods»
            «IF !method.isFireAndForget»
                «generateMethodReplyDeclarations(deploymentAccessor, method, fInterface, counterMap, methodrepliesMap)»
            «ENDIF»
        «ENDFOR»

            virtual ~«fInterface.stubClassName»() {}
            virtual const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> clientId) = 0;

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                /// Provides getter access to the attribute «attribute.elementName»
                «var definition = ""»
                «IF (FTypeGenerator::isdeprecated(attribute.comment))»
                    «{definition = " COMMONAPI_DEPRECATED";""}»
                «ENDIF»
                virtual«definition» const «attribute.getTypeName(fInterface, true)» &«attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                «IF attribute.isObservable»
                    /// sets attribute with the given value and propagates it to the adapter
                    virtual void «attribute.stubAdapterClassFireChangedMethodName»(«attribute.getTypeName(fInterface, true)» _value) {
                    auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                    if (stubAdapter)
                        stubAdapter->«attribute.stubAdapterClassFireChangedMethodName»(_value);
                    }
                «ENDIF»
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                /// This is the method that will be called on remote calls on the method «method.elementName».
                «var definition = ""»
                «IF (FTypeGenerator::isdeprecated(method.comment))»
                    «{definition = " COMMONAPI_DEPRECATED";""}»
                «ENDIF»
                virtual«definition» void «method.elementName»(«generateOverloadedStubSignature(method, methodrepliesMap.get(method))») = 0;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «var definition = ""»
                «IF (FTypeGenerator::isdeprecated(broadcast.comment))»
                    «{definition = " COMMONAPI_DEPRECATED";""}»
                «ENDIF»                «IF broadcast.selective»
                    /**
                     * Sends a selective broadcast event for «broadcast.elementName» to the given ClientIds.
                     * The ClientIds must all be out of the set of subscribed clients.
                     * If no ClientIds are given, the selective broadcast is sent to all subscribed clients.
                     */
                    virtual«definition» void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)») = 0;
                    /// retrieves the list of all subscribed clients for «broadcast.elementName»
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»() {
                        auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                        if (stubAdapter)
                            return(stubAdapter->«broadcast.stubAdapterClassSubscribersMethodName»());
                        else
                            return NULL;
                    }
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) = 0;
                    /// Hook method for reacting accepting or denying new subscriptions
                    virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                    virtual void «broadcast.stubAdapterClassSendSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)») {
                        auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                        if (stubAdapter)
                            stubAdapter->«broadcast.stubAdapterClassSendSelectiveMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»«IF(!broadcast.outArgs.empty)», «ENDIF»_receivers);
                    }
                «ELSE»
                    «IF (!broadcast.isErrorType(deploymentAccessor))»
                        /// Sends a broadcast event for «broadcast.elementName».
                        virtual«definition» void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») {
                            auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                            if (stubAdapter)
                                stubAdapter->«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»);
                        }
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual bool «managed.stubRegisterManagedMethodWithInstanceNumberImpl» {
                    std::stringstream ss;
                    auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                    if (stubAdapter) {
                        ss << stubAdapter->getAddress().getInstance() << ".i" << _instanceNumber;
                        std::string instance = ss.str();
                        return stubAdapter->«managed.stubRegisterManagedName»(_stub, instance);
                    } else {
                        return false;
                    }
                }
                virtual bool «managed.stubRegisterManagedMethodImpl» {
                    auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                    if (stubAdapter) {
                        return stubAdapter->«managed.stubRegisterManagedName»(_stub, _instance);
                    } else {
                        return false;
                    }
                }
                virtual bool «managed.stubDeregisterManagedName»(const std::string& _instance) {
                    auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                    if (stubAdapter)
                        return stubAdapter->«managed.stubDeregisterManagedName»(_instance);
                    else
                        return false;
                }
                virtual std::set<std::string>& «managed.stubManagedSetGetterName»() {
                    auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                    if (stubAdapter) {
                        return stubAdapter->«managed.stubManagedSetGetterName»();
                    } else {
                        static std::set<std::string> emptySet = std::set<std::string>();
                        return emptySet;
                    }
                }
            «ENDFOR»
            
            using «fInterface.stubCommonAPIClassName»::initStubAdapter;
            typedef «fInterface.stubCommonAPIClassName»::StubAdapterType StubAdapterType;
            typedef «fInterface.stubCommonAPIClassName»::RemoteEventHandlerType RemoteEventHandlerType;
            typedef «fInterface.stubRemoteEventClassName» RemoteEventType;
            typedef «fInterface.elementName» StubInterface;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName»_STUB_HPP_
    '''

    def private generateMethodReplyDeclarations(PropertyAccessor deploymentAccessor, FMethod fMethod, FInterface fInterface, HashMap<String, Integer> counterMap,
        HashMap<FMethod, LinkedHashMap<String, Boolean>> methodrepliesMap
    ) '''
        «var replies = methodrepliesMap.get(fMethod)»
        «IF replies == null»
            «{replies = new LinkedHashMap<String, Boolean>();""}»
        «ENDIF»
        «IF !(counterMap.containsKey(fMethod.elementName))»
            «{replies.put(fMethod.elementName, false);""}»
            «{counterMap.put(fMethod.elementName, 0);""}»
                typedef std::function<void («fMethod.generateStubReplySignature()»)>«fMethod.elementName»Reply_t;
        «ELSE»
            «{counterMap.put(fMethod.elementName, counterMap.get(fMethod.elementName) + 1);""}»
            «var reply = fMethod.elementName + counterMap.get(fMethod.elementName)»
            «{replies.put(reply, false);""}»
                typedef std::function<void («fMethod.generateStubReplySignature()»)>«reply»Reply_t;
        «ENDIF»
        «FOR broadcast : fInterface.broadcasts»
            «IF broadcast.isErrorType(fMethod, deploymentAccessor)»
                «val errorReply = fMethod.elementName + broadcast.elementName.toFirstUpper»
                «{replies.put(errorReply, true);""}»
                    typedef std::function<void («broadcast.generateStubErrorReplySignature(deploymentAccessor)»)>«errorReply»Reply_t;
            «ENDIF»
        «ENDFOR»
        «{methodrepliesMap.put(fMethod, replies);""}»
    '''


    def private generateStubDefaultHeader(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «getHeaderDefineName(fInterface)»_HPP_
        #define «getHeaderDefineName(fInterface)»_HPP_

        «IF fInterface.base != null»
            #include <«fInterface.base.stubDefaultHeaderPath»>
        «ENDIF»

        #include <CommonAPI/Export.hpp>

        #include <«fInterface.stubHeaderPath»>
        #include <sstream>

        #  if _MSC_VER >= 1300
        /*
         * Diamond inheritance is used for the CommonAPI::Proxy base class.
         * The Microsoft compiler put warning (C4250) using a desired c++ feature: "Delegating to a sister class"
         * A powerful technique that arises from using virtual inheritance is to delegate a method from a class in another class
         * by using a common abstract base class. This is also called cross delegation.
         */
        #    pragma warning( disable : 4250 )
        #  endif

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        /**
         * Provides a default implementation for «fInterface.stubRemoteEventClassName» and
         * «fInterface.stubClassName». Method callbacks have an empty implementation,
         * remote set calls on attributes will always change the value of the attribute
         * to the one received.
         *
         * Override this stub if you only want to provide a subset of the functionality
         * that would be defined for this service, and/or if you do not need any non-default
         * behaviour.
         */
        class COMMONAPI_EXPORT_CLASS_EXPLICIT «fInterface.stubDefaultClassName»
            : public virtual «fInterface.stubClassName»«IF fInterface.base != null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubDefault«ENDIF» {
        public:
            COMMONAPI_EXPORT «fInterface.stubDefaultClassName»();

            COMMONAPI_EXPORT «fInterface.stubRemoteEventClassName»* initStubAdapter(const std::shared_ptr< «fInterface.stubAdapterClassName»> &_adapter);

            COMMONAPI_EXPORT const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> _client);

            «FOR attribute : fInterface.attributes»
                «val typeName = attribute.getTypeName(fInterface, true)»
                COMMONAPI_EXPORT virtual const «typeName»& «attribute.stubClassGetMethodName»();
                COMMONAPI_EXPORT virtual const «typeName»& «attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client);
                COMMONAPI_EXPORT virtual void «attribute.stubDefaultClassSetMethodName»(«typeName» _value);
                «IF !attribute.readonly»
                    COMMONAPI_EXPORT virtual void «attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value);
                «ENDIF»
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                COMMONAPI_EXPORT virtual void «method.elementName»(«generateOverloadedStubSignature(method, methodrepliesMap?.get(method))»);
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «IF broadcast.selective»
                    COMMONAPI_EXPORT virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)»);
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    COMMONAPI_EXPORT virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event);
                    /// Hook method for reacting accepting or denying new subscriptions
                    COMMONAPI_EXPORT virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client);
                «ELSE»
                    «IF !broadcast.isErrorType(deploymentAccessor)»
                        COMMONAPI_EXPORT virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')»);
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                COMMONAPI_EXPORT bool «managed.stubRegisterManagedAutoName»(std::shared_ptr< «managed.getStubFullClassName»>);
                COMMONAPI_EXPORT «managed.stubRegisterManagedMethod»;
                COMMONAPI_EXPORT bool «managed.stubDeregisterManagedName»(const std::string&);
                COMMONAPI_EXPORT std::set<std::string>& «managed.stubManagedSetGetterName»();
            «ENDFOR»
            
        protected:
            «FOR attribute : fInterface.attributes»
                «val typeName = attribute.getTypeName(fInterface, true)»
                «FTypeGenerator::generateComments(attribute, false)»
                COMMONAPI_EXPORT virtual bool «attribute.stubDefaultClassTrySetMethodName»(«typeName» _value);
                COMMONAPI_EXPORT virtual bool «attribute.stubDefaultClassValidateMethodName»(const «typeName» &_value);
                «IF !attribute.readonly»
                    COMMONAPI_EXPORT virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                «ENDIF»
            «ENDFOR»
            class COMMONAPI_EXPORT_CLASS_EXPLICIT RemoteEventHandler: public virtual «fInterface.stubRemoteEventClassName»«IF fInterface.base != null», public virtual «fInterface.base.stubDefaultClassName»::RemoteEventHandler«ENDIF» {
            public:
                COMMONAPI_EXPORT RemoteEventHandler(«fInterface.stubDefaultClassName» *_defaultStub);

                «FOR attribute : fInterface.attributes»
                    «val typeName = attribute.getTypeName(fInterface, true)»
                    «FTypeGenerator::generateComments(attribute, false)»
                    «IF !attribute.readonly»
                        COMMONAPI_EXPORT virtual bool «attribute.stubRemoteEventClassSetMethodName»(«typeName» _value);
                        COMMONAPI_EXPORT virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value);
                        COMMONAPI_EXPORT virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                    «ENDIF»

                «ENDFOR»

            private:
                «fInterface.stubDefaultClassName» *defaultStub_;
            };
        private:
            «fInterface.stubDefaultClassName»::RemoteEventHandler remoteEventHandler_;
            «IF !fInterface.managedInterfaces.empty»
                uint32_t autoInstanceCounter_;
            «ENDIF»

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                «attribute.getTypeName(fInterface, true)» «attribute.stubDefaultClassVariableName» {};
            «ENDFOR»

            CommonAPI::Version interfaceVersion_;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        «fInterface.generateMajorVersionNamespace»

        #endif // «getHeaderDefineName(fInterface)»
    '''

    def private generateStubDefaultSource(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        #include <«fInterface.stubDefaultHeaderPath»>
        #include <assert.h>

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        «fInterface.stubDefaultClassName»::«fInterface.stubDefaultClassName»():
                remoteEventHandler_(this),
                «IF !fInterface.managedInterfaces.empty»
                    autoInstanceCounter_(0),
                «ENDIF»
                interfaceVersion_(«fInterface.elementName»::getInterfaceVersion()) {
        }

        const CommonAPI::Version& «fInterface.stubDefaultClassName»::getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> _client) {
            (void)_client;
            return interfaceVersion_;
        }

        «fInterface.stubRemoteEventClassName»* «fInterface.stubDefaultClassName»::initStubAdapter(const std::shared_ptr< «fInterface.stubAdapterClassName»> &_adapter) {
            «IF fInterface.base != null»«fInterface.base.stubDefaultClassName»::initStubAdapter(_adapter);«ENDIF»
            «fInterface.stubCommonAPIClassName»::stubAdapter_ = _adapter;
            return &remoteEventHandler_;
        }

        «FOR attribute : fInterface.attributes»
            «val typeName = attribute.getTypeName(fInterface, true)»
            const «typeName»& «fInterface.stubDefaultClassName»::«attribute.stubClassGetMethodName»() {
                return «attribute.stubDefaultClassVariableName»;
            }

            const «typeName»& «fInterface.stubDefaultClassName»::«attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) {
                (void)_client;
                return «attribute.stubClassGetMethodName»();
            }

            void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(«typeName» _value) {
                «IF attribute.isObservable»const bool valueChanged = «ENDIF»«attribute.stubDefaultClassTrySetMethodName»(std::move(_value));
                «IF attribute.isObservable»
                if (valueChanged) {
                    «attribute.stubAdapterClassFireChangedMethodName»(«attribute.stubDefaultClassVariableName»);
                }
                «ENDIF»
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassTrySetMethodName»(«typeName» _value) {
                if (!«attribute.stubDefaultClassValidateMethodName»(_value))
                    return false;

                const bool valueChanged = («attribute.stubDefaultClassVariableName» != _value);
                «attribute.stubDefaultClassVariableName» = std::move(_value);
                return valueChanged;
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassValidateMethodName»(const «typeName» &_value) {
                (void)_value;
                «IF attribute.supportsTypeValidation»
                    return «attribute.validateType(fInterface)»;
                «ELSE»
                    return true;
                «ENDIF»
            }

            «IF !attribute.readonly»
                void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value) {
                    (void)_client;
                    «attribute.stubDefaultClassSetMethodName»(_value);
                }

                void «fInterface.stubDefaultClassName»::«attribute.stubRemoteEventClassChangedMethodName»() {
                    // No operation in default
                }

                void «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassChangedMethodName»() {
                    assert(defaultStub_ !=NULL);
                    defaultStub_->«attribute.stubRemoteEventClassChangedMethodName»();
                }

                bool «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassSetMethodName»(«typeName» _value) {
                    assert(defaultStub_ !=NULL);
                    return defaultStub_->«attribute.stubDefaultClassTrySetMethodName»(std::move(_value));
                }

                bool «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value) {
                    (void)_client;
                    return «attribute.stubRemoteEventClassSetMethodName»(_value);
                }
            «ENDIF»

        «ENDFOR»

        «FOR method : fInterface.methods»
            «FTypeGenerator::generateComments(method, false)»
            void «fInterface.stubDefaultClassName»::«method.elementName»(«generateOverloadedStubSignature(method, methodrepliesMap?.get(method))») {
                (void)_client;
                «IF !method.inArgs.empty»
                    «method.inArgs.map['(void)_' + it.name].join(";\n")»;
                «ENDIF»
                «IF !method.isFireAndForget»
                    «method.generateDummyArgumentDefinitions»
                    «FOR arg : method.outArgs»
                        «IF !arg.array && arg.getType.supportsValidation»
                            if (!«arg.elementName».validate()) {
                                return;
                            }
                        «ENDIF»
                    «ENDFOR»
                    _reply(«method.generateDummyArgumentList»);
                «ENDIF»
            }
            
        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            «FTypeGenerator::generateComments(broadcast, false)»
            «IF broadcast.selective»
                void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, false)») {
                    «FOR arg : broadcast.outArgs»
                        «IF !arg.array && arg.getType.supportsValidation»
                            if (!_«arg.elementName».validate()) {
                                return;
                            }
                        «ENDIF»
                    «ENDFOR»
                    «broadcast.stubAdapterClassSendSelectiveMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»«IF(!broadcast.outArgs.empty)», «ENDIF»_receivers);
                }
                void «fInterface.stubDefaultClassName»::«broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {
                    (void)_client;
                    (void)_event;
                    // No operation in default
                }
                bool «fInterface.stubDefaultClassName»::«broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) {
                    (void)_client;
                    // Accept in default
                    return true;
                }
            «ELSE»
                «IF !broadcast.isErrorType(deploymentAccessor)»
                    void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») {
                        «FOR arg : broadcast.outArgs»
                            «IF !arg.array && arg.getType.supportsValidation»
                                if (!_«arg.elementName».validate()) {
                                    return;
                                }
                            «ENDIF»
                        «ENDFOR»
                        «fInterface.stubClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»);
                    }
                «ENDIF»
            «ENDIF»
        «ENDFOR»

        «FOR managed : fInterface.managedInterfaces»
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedAutoName»(std::shared_ptr< «managed.stubFullClassName»> _stub) {
                autoInstanceCounter_++;
                return «fInterface.stubClassName»::«managed.stubRegisterManagedName»(_stub, autoInstanceCounter_);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedMethodImpl» {
                return «fInterface.stubClassName»::«managed.stubRegisterManagedName»(_stub, _instance);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubDeregisterManagedName»(const std::string &_instance) {
                return «fInterface.stubClassName»::«managed.stubDeregisterManagedName»(_instance);
            }
            std::set<std::string>& «fInterface.stubDefaultClassName»::«managed.stubManagedSetGetterName»() {
                return «fInterface.stubClassName»::«managed.stubManagedSetGetterName»();
            }
        «ENDFOR»

        «fInterface.stubDefaultClassName»::RemoteEventHandler::RemoteEventHandler(«fInterface.stubDefaultClassName» *_defaultStub)
            : «fInterface.generateBaseRemoteHandlerConstructorsCalls»
              defaultStub_(_defaultStub) {
        }

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
    '''

    def private getStubDefaultClassSetMethodName(FAttribute fAttribute) {
        'set' + fAttribute.elementName.toFirstUpper + 'Attribute'
    }

    def private getStubDefaultClassTrySetMethodName(FAttribute fAttribute) {
        'trySet' + fAttribute.elementName.toFirstUpper + 'Attribute'
    }

    def private getStubDefaultClassValidateMethodName(FAttribute fAttribute) {
        'validate' + fAttribute.elementName.toFirstUpper + 'AttributeRequestedValue'
    }

    def private getStubDefaultClassVariableName(FAttribute fAttribute) {
        fAttribute.elementName.toFirstLower + 'AttributeValue_'
    }
}
