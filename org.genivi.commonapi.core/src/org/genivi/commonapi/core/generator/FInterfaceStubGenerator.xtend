/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.HashMap
import java.util.HashSet
import java.util.LinkedHashMap
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants

class FInterfaceStubGenerator {
	@Inject extension FTypeGenerator
	@Inject extension FrancaGeneratorExtensions

    var HashMap<String, Integer> counterMap;
    var HashMap<FMethod, LinkedHashMap<String, Boolean>> methodrepliesMap;

    def generateStub(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        if(FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true").equals("true")) {
            fileSystemAccess.generateFile(fInterface.stubHeaderPath, PreferenceConstants.P_OUTPUT_STUBS, fInterface.generateStubHeader(deploymentAccessor, modelid))
            // should skeleton code be generated ?
            if(FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_SKELETON, "false").equals("true"))
            {
                fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.generateStubDefaultHeader(deploymentAccessor, modelid))
            }
        }
        else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fInterface.stubHeaderPath, PreferenceConstants.P_OUTPUT_STUBS, PreferenceConstants::NO_CODE)
            // should skeleton code be generated ?
            if(FPreferences::instance.getPreference(PreferenceConstants::P_GENERATE_SKELETON, "false").equals("true"))
            {
                fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, PreferenceConstants::NO_CODE)
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

        «startInternalCompilation»

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «IF !fInterface.attributes.empty»
            #include <mutex>
        «ENDIF»

        #include <CommonAPI/Stub.hpp>

        «endInternalCompilation»

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
              public virtual «fInterface.elementName»«IF fInterface.base !== null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubAdapter«ENDIF» {
         public:
            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «IF itsElement.isObservable»
                        ///Notifies all remote listeners about a change of value of the attribute «itsElement.elementName».
                        virtual void «itsElement.stubAdapterClassFireChangedMethodName»(const «itsElement.getTypeName(fInterface, true)» &«itsElement.elementName») = 0;
                    «ENDIF»
                «ELSEIF itsElement instanceof FBroadcast»
                    «IF itsElement.selective»
                        /**
                         * Sends a selective broadcast event for «itsElement.elementName». Should not be called directly.
                         * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                         */
                        virtual void «itsElement.stubAdapterClassFireSelectiveMethodName»(«generateFireSelectiveSignatur(itsElement, fInterface)») = 0;
                        virtual void «itsElement.stubAdapterClassSendSelectiveMethodName»(«generateSendSelectiveSignatur(itsElement, fInterface, true)») = 0;
                        virtual void «itsElement.subscribeSelectiveMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, bool &_success) = 0;
                        virtual void «itsElement.unsubscribeSelectiveMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                        virtual std::shared_ptr<CommonAPI::ClientIdList> const «itsElement.stubAdapterClassSubscribersMethodName»() = 0;
                    «ELSE»
                        «IF (!itsElement.isErrorType(deploymentAccessor))»
                            /**
                            * Sends a broadcast event for «itsElement.elementName». Should not be called directly.
                            * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                            */
                            virtual void «itsElement.stubAdapterClassFireEventMethodName»(«itsElement.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») = 0;
                        «ENDIF»
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual «managed.stubRegisterManagedMethod» = 0;
                virtual bool «managed.stubDeregisterManagedName»(const std::string&) = 0;
                virtual std::set<std::string>& «managed.stubManagedSetGetterName»() = 0;
            «ENDFOR»

            virtual void deactivateManagedInstances() = 0;

            «FOR attribute : fInterface.attributes»
                void «attribute.stubClassLockMethodName»(bool _lockAccess) {
                    if (_lockAccess) {
                        «attribute.stubAdapterAttributeMutexName».lock();
                    } else {
                        «attribute.stubAdapterAttributeMutexName».unlock();
                    }
                }
            «ENDFOR»

        protected:
            /**
             * Defines properties for storing the ClientIds of clients / proxies that have
             * subscribed to the selective broadcasts
             */
            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    std::recursive_mutex «itsElement.stubAdapterAttributeMutexName»;
                «ELSEIF itsElement instanceof FBroadcast»
                    «IF itsElement.selective»
                        std::shared_ptr<CommonAPI::ClientIdList> «itsElement.stubAdapterClassSubscriberListPropertyName»;
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR attribute : fInterface.attributes»
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
        «IF fInterface.base !== null»
            : public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubRemoteEvent
        «ENDIF»
        {
        public:
            virtual ~«fInterface.stubRemoteEventClassName»() { }

            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «val itsAttribute = itsElement»
                    «IF !itsAttribute.readonly»
                        /// Verification callback for remote set requests on the attribute «itsAttribute.elementName»
                        virtual bool «itsAttribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «itsAttribute.getTypeName(fInterface, true)» _value) = 0;
                        /// Action callback for remote set requests on the attribute «itsAttribute.elementName»
                        virtual void «itsAttribute.stubRemoteEventClassChangedMethodName»() = 0;
                    «ENDIF»
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
            : public virtual «fInterface.stubCommonAPIClassName»«IF fInterface.base !== null»,
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
            void lockInterfaceVersionAttribute(bool _lockAccess) { static_cast<void>(_lockAccess); }
            bool hasElement(const uint32_t _id) const {
                «val itsSize = fInterface.rsize»
                «IF itsSize > 0»
                    return (_id < «itsSize»);
                «ELSE»
                    (void)_id; // (_id=«itsSize»)
                    return false;
                «ENDIF»
            }
            virtual const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> _client) = 0;

            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «val itsAttribute = itsElement»
                    «FTypeGenerator::generateComments(itsAttribute, false)»
                    /// Provides getter access to the attribute «itsAttribute.elementName»
                    «var definition = ""»
                    «IF (FTypeGenerator::isdeprecated(itsAttribute.comment))»
                        «{definition = " COMMONAPI_DEPRECATED";""}»
                    «ENDIF»
                    virtual«definition» const «itsAttribute.getTypeName(fInterface, true)» &«itsAttribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                    «IF itsAttribute.isObservable»
                        /// sets attribute with the given value and propagates it to the adapter
                        virtual void «itsAttribute.stubAdapterClassFireChangedMethodName»(«itsAttribute.getTypeName(fInterface, true)» _value) {
                        auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                        if (stubAdapter)
                            stubAdapter->«itsAttribute.stubAdapterClassFireChangedMethodName»(_value);
                        }
                    «ENDIF»
                    void «itsAttribute.stubClassLockMethodName»(bool _lockAccess) {
                        auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                        if (stubAdapter)
                            stubAdapter->«itsAttribute.stubClassLockMethodName»(_lockAccess);
                    }
               «ELSEIF itsElement instanceof FMethod»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    /// This is the method that will be called on remote calls on the method «itsElement.elementName».
                    «var definition = ""»
                    «IF (FTypeGenerator::isdeprecated(itsElement.comment))»
                        «{definition = " COMMONAPI_DEPRECATED";""}»
                    «ENDIF»
                    virtual«definition» void «itsElement.elementName»(«generateOverloadedStubSignature(itsElement, methodrepliesMap.get(itsElement))») = 0;
                «ELSEIF itsElement instanceof FBroadcast»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    «var definition = ""»
                    «IF (FTypeGenerator::isdeprecated(itsElement.comment))»
                        «{definition = " COMMONAPI_DEPRECATED";""}»
                    «ENDIF»
                    «IF itsElement.selective»
                        /**
                         * Sends a selective broadcast event for «itsElement.elementName» to the given ClientIds.
                         * The ClientIds must all be out of the set of subscribed clients.
                         * If no ClientIds are given, the selective broadcast is sent to all subscribed clients.
                         */
                        virtual«definition» void «itsElement.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(itsElement, fInterface, true)») = 0;
                        /// retrieves the list of all subscribed clients for «itsElement.elementName»
                        virtual std::shared_ptr<CommonAPI::ClientIdList> const «itsElement.stubAdapterClassSubscribersMethodName»() {
                            auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                            if (stubAdapter)
                                return(stubAdapter->«itsElement.stubAdapterClassSubscribersMethodName»());
                            else
                                return NULL;
                        }
                        /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                        virtual void «itsElement.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) = 0;
                        /// Hook method for reacting accepting or denying new subscriptions
                        virtual bool «itsElement.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                        virtual void «itsElement.stubAdapterClassSendSelectiveMethodName»(«generateSendSelectiveSignatur(itsElement, fInterface, true)») {
                            auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                            if (stubAdapter)
                                stubAdapter->«itsElement.stubAdapterClassSendSelectiveMethodName»(«itsElement.outArgs.map["_" + elementName].join(', ')»«IF(!itsElement.outArgs.empty)», «ENDIF»_receivers);
                        }
                    «ELSE»
                        «IF (!itsElement.isErrorType(deploymentAccessor))»
                            /// Sends a broadcast event for «itsElement.elementName».
                            virtual«definition» void «itsElement.stubAdapterClassFireEventMethodName»(«itsElement.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») {
                                auto stubAdapter = «fInterface.stubCommonAPIClassName»::stubAdapter_.lock();
                                if (stubAdapter)
                                    stubAdapter->«itsElement.stubAdapterClassFireEventMethodName»(«itsElement.outArgs.map["_" + elementName].join(', ')»);
                            }
                        «ENDIF»
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
            typedef «fInterface.versionPrefix»«fInterface.model.generateCppNamespace»«fInterface.elementName» StubInterface;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName»_STUB_HPP_
    '''

    def private generateMethodReplyDeclarations(PropertyAccessor deploymentAccessor, FMethod fMethod, FInterface fInterface, HashMap<String, Integer> counterMap,
        HashMap<FMethod, LinkedHashMap<String, Boolean>> methodrepliesMap) '''
        «var replies = methodrepliesMap.get(fMethod)»
        «IF replies === null»
            «{replies = new LinkedHashMap<String, Boolean>();""}»
        «ENDIF»
        «IF !(counterMap.containsKey(fMethod.elementName))»
            «{replies.put(fMethod.elementName, false);""}»
            «{counterMap.put(fMethod.elementName, 0);""}»
                typedef std::function<void («fMethod.generateStubReplySignature()»)> «fMethod.elementName»Reply_t;
        «ELSE»
            «{counterMap.put(fMethod.elementName, counterMap.get(fMethod.elementName) + 1);""}»
            «var reply = fMethod.elementName + counterMap.get(fMethod.elementName)»
            «{replies.put(reply, false);""}»
                typedef std::function<void («fMethod.generateStubReplySignature()»)> «reply»Reply_t;
        «ENDIF»
        «FOR broadcast : fInterface.broadcasts»
            «IF broadcast.isErrorType(fMethod, deploymentAccessor)»
                «val errorReply = fMethod.elementName + broadcast.elementName.toFirstUpper»
                «{replies.put(errorReply, true);""}»
                    typedef std::function<void («broadcast.generateStubErrorReplySignature(deploymentAccessor)»)> «errorReply»Reply_t;
            «ENDIF»
        «ENDFOR»
        «{methodrepliesMap.put(fMethod, replies);""}»
    '''


    def private generateStubDefaultHeader(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «getHeaderDefineName(fInterface)»_HPP_
        #define «getHeaderDefineName(fInterface)»_HPP_

        «IF fInterface.base !== null»
            #include <«fInterface.base.stubDefaultHeaderPath»>
        «ENDIF»

        #include <CommonAPI/Export.hpp>

        #include <«fInterface.stubHeaderPath»>
        #include <cassert>
        #include <sstream>

        # if defined(_MSC_VER)
        #  if _MSC_VER >= 1300
        /*
         * Diamond inheritance is used for the CommonAPI::Proxy base class.
         * The Microsoft compiler put warning (C4250) using a desired c++ feature: "Delegating to a sister class"
         * A powerful technique that arises from using virtual inheritance is to delegate a method from a class in another class
         * by using a common abstract base class. This is also called cross delegation.
         */
        #    pragma warning( disable : 4250 )
        #  endif
        # endif

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
            : public virtual «fInterface.stubClassName»«IF fInterface.base !== null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubDefault«ENDIF» {
        public:
            COMMONAPI_EXPORT «fInterface.stubDefaultClassName»()
                : remoteEventHandler_(this),
                  «IF !fInterface.managedInterfaces.empty»
                      autoInstanceCounter_(0),
                  «ENDIF»
                  interfaceVersion_(«fInterface.elementName»::getInterfaceVersion()) {
            }

            COMMONAPI_EXPORT const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> _client) {
                (void)_client;
                return interfaceVersion_;
            }

            COMMONAPI_EXPORT «fInterface.stubRemoteEventClassName»* initStubAdapter(const std::shared_ptr< «fInterface.stubAdapterClassName»> &_adapter) {
                «IF fInterface.base !== null»«fInterface.base.stubDefaultClassName»::initStubAdapter(_adapter);«ENDIF»
                «fInterface.stubCommonAPIClassName»::stubAdapter_ = _adapter;
                return &remoteEventHandler_;
            }

            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «val itsType = itsElement.getTypeName(fInterface, true)»
                    COMMONAPI_EXPORT virtual const «itsType» &«itsElement.stubClassGetMethodName»() {
                        return «itsElement.stubDefaultClassVariableName»;
                    }
                    COMMONAPI_EXPORT virtual const «itsType» &«itsElement.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) {
                        (void)_client;
                        return «itsElement.stubClassGetMethodName»();
                    }
                    COMMONAPI_EXPORT virtual void «itsElement.stubDefaultClassSetMethodName»(«itsType» _value) {
                        «IF itsElement.isObservable»
                        const bool valueChanged = «itsElement.stubDefaultClassTrySetMethodName»(std::move(_value));
                        if (valueChanged) {
                            «itsElement.stubAdapterClassFireChangedMethodName»(«itsElement.stubDefaultClassVariableName»);
                        }
                        «ELSE»
                        (void)«itsElement.stubDefaultClassTrySetMethodName»(_value);
                        «ENDIF»
                    }
                    «IF !itsElement.readonly»
                        COMMONAPI_EXPORT virtual void «itsElement.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «itsType» _value) {
                            (void)_client;
                            «itsElement.stubDefaultClassSetMethodName»(_value);
                        }
                    «ENDIF»
                «ELSEIF itsElement instanceof FMethod»
                    «var itsReplies = methodrepliesMap?.get(itsElement)»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    COMMONAPI_EXPORT virtual void «itsElement.elementName»(«generateOverloadedStubSignature(itsElement, methodrepliesMap?.get(itsElement))») {
                        (void)_client;
                        «IF !itsElement.inArgs.empty»
                            «itsElement.inArgs.map['(void)_' + it.name].join(";\n")»;
                        «ENDIF»
                        «IF !itsElement.isFireAndForget»
                            «IF itsReplies !== null && itsReplies.containsValue(true)»
                                (void)_call;
                                «FOR reply : itsReplies.entrySet»
                                    «var methodName = reply.key»
                                    «var isErrorReply = reply.value»
                                    «IF isErrorReply»
                                        (void)_«methodName»Reply;
                                    «ENDIF»
                                «ENDFOR»
                            «ENDIF»
                            «itsElement.generateDummyArgumentDefinitions»
                            «FOR arg : itsElement.outArgs»
                                «IF !arg.array && arg.getType.supportsValidation»
                                    if (!«arg.elementName».validate()) {
                                        return;
                                    }
                                «ENDIF»
                            «ENDFOR»
                            _reply(«itsElement.generateDummyArgumentList»);
                        «ENDIF»
                    }
                «ELSEIF itsElement instanceof FBroadcast»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    «IF itsElement.selective»
                        COMMONAPI_EXPORT virtual void «itsElement.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(itsElement, fInterface, true)») {
                            «FOR arg : itsElement.outArgs»
                                «IF !arg.array && arg.getType.supportsValidation»
                                    if (!_«arg.elementName».validate()) {
                                        return;
                                    }
                                «ENDIF»
                            «ENDFOR»
                            «itsElement.stubAdapterClassSendSelectiveMethodName»(«itsElement.outArgs.map["_" + elementName].join(', ')»«IF(!itsElement.outArgs.empty)», «ENDIF»_receivers);
                        }
                        /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                        COMMONAPI_EXPORT virtual void «itsElement.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {
                            (void)_client;
                            (void)_event;
                            // No operation in default
                        }
                        /// Hook method for reacting accepting or denying new subscriptions
                        COMMONAPI_EXPORT virtual bool «itsElement.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) {
                            (void)_client;
                            // Accept in default
                            return true;
                        }
                    «ELSE»
                        «IF !itsElement.isErrorType(deploymentAccessor)»
                            COMMONAPI_EXPORT virtual void «itsElement.stubAdapterClassFireEventMethodName»(«itsElement.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») {
                                «FOR arg : itsElement.outArgs»
                                    «IF !arg.array && arg.getType.supportsValidation»
                                        if (!_«arg.elementName».validate()) {
                                            return;
                                        }
                                    «ENDIF»
                                «ENDFOR»
                                «fInterface.stubClassName»::«itsElement.stubAdapterClassFireEventMethodName»(«itsElement.outArgs.map["_" + elementName].join(', ')»);
                            }
                        «ENDIF»
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                COMMONAPI_EXPORT bool «managed.stubRegisterManagedAutoName»(std::shared_ptr< «managed.stubFullClassName»> _stub) {
                    autoInstanceCounter_++;
                    return «fInterface.stubClassName»::«managed.stubRegisterManagedName»(_stub, autoInstanceCounter_);
                }
                COMMONAPI_EXPORT bool «managed.stubRegisterManagedMethodImpl» {
                    return «fInterface.stubClassName»::«managed.stubRegisterManagedName»(_stub, _instance);
                }
                COMMONAPI_EXPORT bool «managed.stubDeregisterManagedName»(const std::string &_instance) {
                    return «fInterface.stubClassName»::«managed.stubDeregisterManagedName»(_instance);
                }
                COMMONAPI_EXPORT std::set<std::string>& «managed.stubManagedSetGetterName»() {
                    return «fInterface.stubClassName»::«managed.stubManagedSetGetterName»();
                }
            «ENDFOR»

        protected:
            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «val itsAttribute = itsElement»
                    «val itsType = itsAttribute.getTypeName(fInterface, true)»
                    «FTypeGenerator::generateComments(itsAttribute, false)»
                    COMMONAPI_EXPORT virtual bool «itsAttribute.stubDefaultClassTrySetMethodName»(«itsType» _value) {
                        if (!«itsAttribute.stubDefaultClassValidateMethodName»(_value))
                            return false;

                        bool valueChanged;
                        std::shared_ptr<«fInterface.stubAdapterClassName»> stubAdapter = CommonAPI::Stub<«fInterface.stubAdapterClassName», «fInterface.stubRemoteEventClassName»>::stubAdapter_.lock();
                        if(stubAdapter) {
                            stubAdapter->«itsAttribute.stubClassLockMethodName»(true);
                            valueChanged = («itsAttribute.stubDefaultClassVariableName» != _value);
                            «itsAttribute.stubDefaultClassVariableName» = std::move(_value);
                            stubAdapter->«itsAttribute.stubClassLockMethodName»(false);
                        } else {
                            valueChanged = («itsAttribute.stubDefaultClassVariableName» != _value);
                            «itsAttribute.stubDefaultClassVariableName» = std::move(_value);
                        }

                       return valueChanged;
                    }
                    COMMONAPI_EXPORT virtual bool «itsAttribute.stubDefaultClassValidateMethodName»(const «itsType» &_value) {
                        (void)_value;
                        «IF itsAttribute.supportsTypeValidation»
                            return «itsAttribute.validateType(fInterface)»;
                        «ELSE»
                            return true;
                        «ENDIF»
                    }
                    «IF !itsAttribute.readonly»
                        COMMONAPI_EXPORT virtual void «itsAttribute.stubRemoteEventClassChangedMethodName»() {
                            // No operation in default
                        }
                    «ENDIF»
                «ENDIF»
            «ENDFOR»
            class COMMONAPI_EXPORT_CLASS_EXPLICIT RemoteEventHandler: public virtual «fInterface.stubRemoteEventClassName»«IF fInterface.base !== null», public virtual «fInterface.base.stubDefaultClassName»::RemoteEventHandler«ENDIF» {
            public:
                COMMONAPI_EXPORT RemoteEventHandler(«fInterface.stubDefaultClassName» *_defaultStub)
                    : «fInterface.generateBaseRemoteHandlerConstructorsCalls»
                      defaultStub_(_defaultStub) {
                }

                «FOR itsElement : fInterface.elements»
                    «IF itsElement instanceof FAttribute»
                        «val itsAttribute = itsElement»
                        «val itsType = itsAttribute.getTypeName(fInterface, true)»
                        «FTypeGenerator::generateComments(itsAttribute, false)»
                        «IF !itsAttribute.readonly»
                            COMMONAPI_EXPORT virtual void «itsAttribute.stubRemoteEventClassChangedMethodName»() {
                                assert(defaultStub_ !=NULL);
                                defaultStub_->«itsAttribute.stubRemoteEventClassChangedMethodName»();
                            }

                            COMMONAPI_EXPORT virtual bool «itsAttribute.stubRemoteEventClassSetMethodName»(«itsType» _value) {
                                assert(defaultStub_ !=NULL);
                                return defaultStub_->«itsAttribute.stubDefaultClassTrySetMethodName»(std::move(_value));
                            }

                            COMMONAPI_EXPORT virtual bool «itsAttribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «itsType» _value) {
                                (void)_client;
                                return «itsAttribute.stubRemoteEventClassSetMethodName»(_value);
                            }
                        «ENDIF»
                    «ENDIF»
                «ENDFOR»

            private:
                «fInterface.stubDefaultClassName» *defaultStub_;
            };
        protected:
            «fInterface.stubDefaultClassName»::RemoteEventHandler remoteEventHandler_;

        private:
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

    def private getStubAdapterAttributeMutexName(FAttribute fAttribute) {
        fAttribute.elementName.toFirstLower + 'Mutex_'
    }
}
