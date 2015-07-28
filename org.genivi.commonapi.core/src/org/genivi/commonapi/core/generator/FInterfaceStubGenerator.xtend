/* Copyright (C) 2013, 2014 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * Author: Lutz Bichler (lutz.bichler@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import org.genivi.commonapi.core.preferences.PreferenceConstants
import org.genivi.commonapi.core.preferences.FPreferences
import java.util.HashMap
import org.franca.core.franca.FMethod

class FInterfaceStubGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

    var HashMap<String, Integer> counterMap;
    var HashMap<FMethod, String> methodreplyMap;


    def generateStub(FInterface fInterface, IFileSystemAccess fileSystemAccess, IResource modelid) {
        fileSystemAccess.generateFile(fInterface.stubHeaderPath, PreferenceConstants.P_OUTPUT_STUBS, fInterface.generateStubHeader(modelid))
        // should skeleton code be generated ?
        if(FPreferences::instance.getPreference(PreferenceConstants::P_GENERATESKELETON, "false").equals("true"))
        {
	        fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.generateStubDefaultHeader(modelid))
    	    fileSystemAccess.generateFile(fInterface.stubDefaultSourcePath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.generateStubDefaultSource(modelid))
        }        
    }

    def private generateStubHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_STUB_HPP_
        #define «fInterface.defineName»_STUB_HPP_

        #include <functional>

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»

        «fInterface.generateInheritanceIncludes(generatedHeaders, libraryHeaders)»
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders)»
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
            : virtual public CommonAPI::StubAdapter, 
              public «fInterface.elementName»«IF fInterface.base != null», 
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
                    /**
                     * Sends a broadcast event for «broadcast.elementName». Should not be called directly.
                     * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                     */
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») = 0;
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
                    virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «attribute.getTypeName(fInterface, true)» «attribute.elementName») = 0;
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
        «{methodreplyMap = new HashMap<FMethod, String>();""}»
        «FOR method: fInterface.methods»
            «IF !method.isFireAndForget»
	            «generateMethodReplyDeclarations(method, fInterface, counterMap, methodreplyMap)»
            «ENDIF»
	    «ENDFOR»
        
            virtual ~«fInterface.stubClassName»() {}
            virtual const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> clientId) = 0;

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                /// Provides getter access to the attribute «attribute.elementName»
                virtual const «attribute.getTypeName(fInterface, true)» &«attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                /// This is the method that will be called on remote calls on the method «method.elementName».
                virtual void «method.elementName»(«generateOverloadedStubSignature(method, methodreplyMap.get(method))») = 0;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «IF broadcast.selective»
                    /**
                     * Sends a selective broadcast event for «broadcast.elementName» to the given ClientIds.
                     * The ClientIds must all be out of the set of subscribed clients.
                     * If no ClientIds are given, the selective broadcast is sent to all subscribed clients.
                     */
                    virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)») = 0;
                    /// retreives the list of all subscribed clients for «broadcast.elementName»
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»() = 0;
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) = 0;
                    /// Hook method for reacting accepting or denying new subscriptions 
                    virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) = 0;
                «ELSE»
                    /// Sends a broadcast event for «broadcast.elementName».
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») = 0;
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual «managed.stubRegisterManagedMethod» = 0;
                virtual bool «managed.stubDeregisterManagedName»(const std::string&) = 0;
                virtual std::set<std::string>& «managed.stubManagedSetGetterName»() = 0;
            «ENDFOR»
            using «fInterface.stubCommonAPIClassName»::initStubAdapter;
            typedef «fInterface.stubCommonAPIClassName»::StubAdapterType StubAdapterType;
            typedef «fInterface.stubCommonAPIClassName»::RemoteEventHandlerType RemoteEventHandlerType;
            typedef «fInterface.stubRemoteEventClassName» RemoteEventType;
            typedef «fInterface.elementName» StubInterface;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        #endif // «fInterface.defineName»_STUB_HPP_
    '''

    def private generateMethodReplyDeclarations(FMethod fMethod, FInterface fInterface, HashMap<String, Integer> counterMap, HashMap<FMethod, String> methodreplyMap) '''
		«IF !(counterMap.containsKey(fMethod.elementName))»
			«{counterMap.put(fMethod.elementName, 0);  methodreplyMap.put(fMethod, fMethod.elementName);""}»
			    typedef std::function<void («fMethod.generateStubReplySignature()»)>«fMethod.elementName»Reply_t;
		«ELSE»
			«{counterMap.put(fMethod.elementName, counterMap.get(fMethod.elementName) + 1);  methodreplyMap.put(fMethod, fMethod.elementName + counterMap.get(fMethod.elementName));""}»
			    typedef std::function<void («fMethod.generateStubReplySignature()»)>«methodreplyMap.get(fMethod)»Reply_t;
		«ENDIF»
    '''


    def private generateStubDefaultHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «getHeaderDefineName(fInterface)»_HPP_
        #define «getHeaderDefineName(fInterface)»_HPP_

        «IF fInterface.base != null»
            #include <«fInterface.base.stubDefaultHeaderPath»>
        «ENDIF»

        #include <«fInterface.stubHeaderPath»>
        #include <sstream>

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
        class «fInterface.stubDefaultClassName»
            : public virtual «fInterface.stubClassName»«IF fInterface.base != null»,
              public virtual «fInterface.base.getTypeCollectionName(fInterface)»StubDefault«ENDIF» {
        public:
            «fInterface.stubDefaultClassName»();

            «fInterface.stubRemoteEventClassName»* initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»> &_adapter);

            const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> _client);

            «FOR attribute : fInterface.attributes»
                «val typeName = attribute.getTypeName(fInterface, true)»
                virtual const «typeName»& «attribute.stubClassGetMethodName»();
                virtual const «typeName»& «attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client);
                virtual void «attribute.stubDefaultClassSetMethodName»(«typeName» _value);
                «IF !attribute.readonly»
                    virtual void «attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value);
                «ENDIF»
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                virtual void «method.elementName»(«generateOverloadedStubSignature(method, methodreplyMap?.get(method))»);
            «ENDFOR»

          	«FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «IF broadcast.selective»
                    virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)»);
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»();
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event);
                    /// Hook method for reacting accepting or denying new subscriptions 
                    virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client);
                «ELSE»
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')»);
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                bool «managed.stubRegisterManagedAutoName»(std::shared_ptr<«managed.stubClassName»>);
                «managed.stubRegisterManagedMethod»;
                bool «managed.stubDeregisterManagedName»(const std::string&);
                std::set<std::string>& «managed.stubManagedSetGetterName»();
            «ENDFOR»

        protected:
            «FOR attribute : fInterface.attributes»
                «val typeName = attribute.getTypeName(fInterface, true)»
                «FTypeGenerator::generateComments(attribute, false)»
                virtual bool «attribute.stubDefaultClassTrySetMethodName»(«typeName» _value);
                virtual bool «attribute.stubDefaultClassValidateMethodName»(const «typeName» &_value);
                «IF !attribute.readonly»
                    virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                «ENDIF»
            «ENDFOR»
            class RemoteEventHandler: public virtual «fInterface.stubRemoteEventClassName»«IF fInterface.base != null», public virtual «fInterface.base.stubDefaultClassName»::RemoteEventHandler«ENDIF» {
            public:
                RemoteEventHandler(«fInterface.stubDefaultClassName» *_defaultStub);

                «FOR attribute : fInterface.attributes»
                    «val typeName = attribute.getTypeName(fInterface, true)»
                    «FTypeGenerator::generateComments(attribute, false)»
                    «IF !attribute.readonly»
                        virtual bool «attribute.stubRemoteEventClassSetMethodName»(«typeName» _value);
                        virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value);
                        virtual void «attribute.stubRemoteEventClassChangedMethodName»();
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
                «attribute.getTypeName(fInterface, true)» «attribute.stubDefaultClassVariableName»;
            «ENDFOR»            

            CommonAPI::Version interfaceVersion_;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        #endif // «getHeaderDefineName(fInterface)»
    '''

    def private generateStubDefaultSource(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
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
            return interfaceVersion_;
        }

        «fInterface.stubRemoteEventClassName»* «fInterface.stubDefaultClassName»::initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»> &_adapter) {
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
                return «attribute.stubClassGetMethodName»();
            }

            void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(«typeName» _value) {
                «IF attribute.isObservable»const bool valueChanged = «ENDIF»«attribute.stubDefaultClassTrySetMethodName»(std::move(_value));
                «IF attribute.isObservable»
                   if (valueChanged && «fInterface.stubCommonAPIClassName»::stubAdapter_ != NULL) {
                        «fInterface.stubCommonAPIClassName»::stubAdapter_->«attribute.stubAdapterClassFireChangedMethodName»(«attribute.stubDefaultClassVariableName»);
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
                return true;
            }

            «IF !attribute.readonly»
                void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, «typeName» _value) {
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
                    return «attribute.stubRemoteEventClassSetMethodName»(_value);
                }
            «ENDIF»

        «ENDFOR»

        «FOR method : fInterface.methods»
            «FTypeGenerator::generateComments(method, false)»
            void «fInterface.stubDefaultClassName»::«method.elementName»(«generateOverloadedStubSignature(method, methodreplyMap?.get(method))») {
                «IF !method.isFireAndForget»
                    «method.generateDummyArgumentDefinitions»
                    _reply(«method.generateDummyArgumentList»);
                «ENDIF»
            }
            
        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            «FTypeGenerator::generateComments(broadcast, false)»
            «IF broadcast.selective»
                void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, false)») {
                	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                    «fInterface.stubCommonAPIClassName»::stubAdapter_->«broadcast.stubAdapterClassSendSelectiveMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»«IF(!broadcast.outArgs.empty)», «ENDIF»_receivers);
                }
                void «fInterface.stubDefaultClassName»::«broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client, const CommonAPI::SelectiveBroadcastSubscriptionEvent _event) {
                    // No operation in default
                }
                bool «fInterface.stubDefaultClassName»::«broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> _client) {
                    // Accept in default
                    return true;
                }
                std::shared_ptr<CommonAPI::ClientIdList> const «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassSubscribersMethodName»() {
                	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                    return(«fInterface.stubCommonAPIClassName»::stubAdapter_->«broadcast.stubAdapterClassSubscribersMethodName»());
                }

            «ELSE»
                void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface, true) + ' &_' + elementName].join(', ')») {
                	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                    «fInterface.stubCommonAPIClassName»::stubAdapter_->«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map["_" + elementName].join(', ')»);
                }
            «ENDIF»
        «ENDFOR»

        «FOR managed : fInterface.managedInterfaces»
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedAutoName»(std::shared_ptr<«managed.stubClassName»> _stub) {
                autoInstanceCounter_++;
                std::stringstream ss;
                ss << «fInterface.stubCommonAPIClassName»::stubAdapter_->getAddress().getInstance() << ".i" << autoInstanceCounter_;
                std::string instance = ss.str();
                assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                return «fInterface.stubCommonAPIClassName»::stubAdapter_->«managed.stubRegisterManagedName»(_stub, instance);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedMethodImpl» {
            	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                return «fInterface.stubCommonAPIClassName»::stubAdapter_->«managed.stubRegisterManagedName»(_stub, _instance);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubDeregisterManagedName»(const std::string &_instance) {
            	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                return «fInterface.stubCommonAPIClassName»::stubAdapter_->«managed.stubDeregisterManagedName»(_instance);
            }
            std::set<std::string>& «fInterface.stubDefaultClassName»::«managed.stubManagedSetGetterName»() {
            	assert((«fInterface.stubCommonAPIClassName»::stubAdapter_) !=NULL);
                return «fInterface.stubCommonAPIClassName»::stubAdapter_->«managed.stubManagedSetGetterName»();
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
