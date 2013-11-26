/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import java.util.HashSet
import org.eclipse.core.resources.IResource

class FInterfaceStubGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

    def generateStub(FInterface fInterface, IFileSystemAccess fileSystemAccess, IResource modelid) {
        fileSystemAccess.generateFile(fInterface.stubHeaderPath, fInterface.generateStubHeader(modelid))
        fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, fInterface.generateStubDefaultHeader(modelid))
        fileSystemAccess.generateFile(fInterface.stubDefaultSourcePath, fInterface.generateStubDefaultSource(modelid))
    }

    def private generateStubHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_STUB_H_
        #define «fInterface.defineName»_STUB_H_

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»

        «fInterface.generateInheritanceIncludes(generatedHeaders, libraryHeaders)»
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders)»
        «fInterface.generateSelectiveBroadcastStubIncludes(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #include "«fInterface.elementName».h"

        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #include <CommonAPI/Stub.h>

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.model.generateNamespaceBeginDeclaration»

        /**
         * Receives messages from remote and handles all dispatching of deserialized calls
         * to a stub for the service «fInterface.elementName». Also provides means to send broadcasts
         * and attribute-changed-notifications of observable attributes as defined by this service.
         * An application developer should not need to bother with this class.
         */
        class «fInterface.stubAdapterClassName»: virtual public CommonAPI::StubAdapter, public «fInterface.elementName» {
         public:
            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    ///Notifies all remote listeners about a change of value of the attribute «attribute.elementName».
                    virtual void «attribute.stubAdapterClassFireChangedMethodName»(const «attribute.getTypeName(fInterface.model)»& «attribute.elementName») = 0;
                «ENDIF»
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «IF !broadcast.selective.nullOrEmpty»
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
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface.model) + '& ' + elementName].join(', ')») = 0;
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
                «IF broadcast.selective != null»
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
        class «fInterface.stubRemoteEventClassName» {
         public:
            virtual ~«fInterface.stubRemoteEventClassName»() { }

            «FOR attribute : fInterface.attributes»
                «IF !attribute.readonly»
                    /// Verification callback for remote set requests on the attribute «attribute.elementName»
                    virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, «attribute.getTypeName(fInterface.model)» «attribute.elementName») = 0;
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
        class «fInterface.stubClassName» : public virtual «fInterface.stubCommonAPIClassName»«IF fInterface.base != null», public virtual «fInterface.base.stubClassName»«ENDIF» {
        public:
            virtual ~«fInterface.stubClassName»() { }
            virtual const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> clientId) = 0;

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                /// Provides getter access to the attribute «attribute.elementName»
                virtual const «attribute.getTypeName(fInterface.model)»& «attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId) = 0;
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                /// This is the method that will be called on remote calls on the method «method.elementName».
                virtual void «method.elementName»(«method.generateStubSignature») = 0;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «IF !broadcast.selective.nullOrEmpty»
                    /**
                     * Sends a selective broadcast event for «broadcast.elementName» to the given ClientIds.
                     * The ClientIds must all be out of the set of subscribed clients.
                     * If no ClientIds are given, the selective broadcast is sent to all subscribed clients.
                     */
                    virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)») = 0;
                    /// retreives the list of all subscribed clients for «broadcast.elementName»
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»() = 0;
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, const CommonAPI::SelectiveBroadcastSubscriptionEvent event) = 0;
                    /// Hook method for reacting accepting or denying new subscriptions 
                    virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId) = 0;
                «ELSE»
                    /// Sends a broadcast event for «broadcast.elementName».
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface.model) + '& ' + elementName].join(', ')») = 0;
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
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_STUB_H_
    '''

    def private generateStubDefaultHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_STUB_DEFAULT_H_
        #define «fInterface.defineName»_STUB_DEFAULT_H_

        «IF fInterface.base != null»
            #include <«fInterface.base.stubDefaultHeaderPath»>
        «ENDIF»

        #include <«fInterface.stubHeaderPath»>
        #include <sstream>

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
        class «fInterface.stubDefaultClassName» : public virtual «fInterface.stubClassName»«IF fInterface.base != null», public virtual «fInterface.base.stubDefaultClassName»«ENDIF» {
        public:
            «fInterface.stubDefaultClassName»();

            «fInterface.stubRemoteEventClassName»* initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»>& stubAdapter);

            const CommonAPI::Version& getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> clientId);

            «FOR attribute : fInterface.attributes»
                virtual const «attribute.getTypeName(fInterface.model)»& «attribute.stubClassGetMethodName»();
                virtual const «attribute.getTypeName(fInterface.model)»& «attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId);
                virtual void «attribute.stubDefaultClassSetMethodName»(«attribute.getTypeName(fInterface.model)» value);
                «IF !attribute.readonly»
                    virtual void «attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, «attribute.getTypeName(fInterface.model)» value);
                «ENDIF»
            «ENDFOR»

            «FOR method: fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
                virtual void «method.elementName»(«method.generateStubSignature»);
                virtual void «method.elementName»(«method.generateStubSignatureOldStyle»);

            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                «IF !broadcast.selective.nullOrEmpty»
                    virtual void «broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, true)»);
                    virtual std::shared_ptr<CommonAPI::ClientIdList> const «broadcast.stubAdapterClassSubscribersMethodName»();
                    /// Hook method for reacting on new subscriptions or removed subscriptions respectively for selective broadcasts.
                    virtual void «broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, const CommonAPI::SelectiveBroadcastSubscriptionEvent event);
                    /// Hook method for reacting accepting or denying new subscriptions 
                    virtual bool «broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId);
                «ELSE»
                    virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface.model) + '& ' + elementName].join(', ')»);
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
                «FTypeGenerator::generateComments(attribute, false)»
                virtual bool «attribute.stubDefaultClassTrySetMethodName»(«attribute.getTypeName(fInterface.model)» value);
                virtual bool «attribute.stubDefaultClassValidateMethodName»(const «attribute.getTypeName(fInterface.model)»& value);
                «IF !attribute.readonly»
                    virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                «ENDIF»
            «ENDFOR»
        private:
            class RemoteEventHandler: public «fInterface.stubRemoteEventClassName» {
             public:
                RemoteEventHandler(«fInterface.stubDefaultClassName»* defaultStub);

                «FOR attribute : fInterface.attributes»
                    «FTypeGenerator::generateComments(attribute, false)»
                    «IF !attribute.readonly»
                        virtual bool «attribute.stubRemoteEventClassSetMethodName»(«attribute.getTypeName(fInterface.model)» value);
                        virtual bool «attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, «attribute.getTypeName(fInterface.model)» value);
                        virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                    «ENDIF»

                «ENDFOR»

             private:
                «fInterface.stubDefaultClassName»* defaultStub_;
            };

            RemoteEventHandler remoteEventHandler_;
            «IF !fInterface.managedInterfaces.empty»
                uint32_t autoInstanceCounter_;
            «ENDIF»

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                «attribute.getTypeName(fInterface.model)» «attribute.stubDefaultClassVariableName»;
            «ENDFOR»            

            CommonAPI::Version interfaceVersion_;
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_STUB_DEFAULT_H_
    '''

    def private generateStubDefaultSource(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        #include <«fInterface.stubDefaultHeaderPath»>

        «fInterface.model.generateNamespaceBeginDeclaration»

        «fInterface.stubDefaultClassName»::«fInterface.stubDefaultClassName»():
                «IF !fInterface.managedInterfaces.empty»
                    autoInstanceCounter_(0),
                «ENDIF»
                remoteEventHandler_(this),
                interfaceVersion_(«fInterface.elementName»::getInterfaceVersion()) {
        }

        const CommonAPI::Version& «fInterface.stubDefaultClassName»::getInterfaceVersion(std::shared_ptr<CommonAPI::ClientId> clientId) {
            return interfaceVersion_;
        }

        «fInterface.stubRemoteEventClassName»* «fInterface.stubDefaultClassName»::initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»>& stubAdapter) {
            «fInterface.stubCommonAPIClassName»::stubAdapter_ = stubAdapter;
            return &remoteEventHandler_;
        }

        «FOR attribute : fInterface.attributes»
            const «attribute.getTypeName(fInterface.model)»& «fInterface.stubDefaultClassName»::«attribute.stubClassGetMethodName»() {
                return «attribute.stubDefaultClassVariableName»;
            }

            const «attribute.getTypeName(fInterface.model)»& «fInterface.stubDefaultClassName»::«attribute.stubClassGetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId) {
                return «attribute.stubClassGetMethodName»();
            }

            void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(«attribute.getTypeName(fInterface.model)» value) {
                «IF attribute.isObservable»const bool valueChanged = «ENDIF»«attribute.stubDefaultClassTrySetMethodName»(std::move(value));
                «IF attribute.isObservable»
                    if (valueChanged) {
                        stubAdapter_->«attribute.stubAdapterClassFireChangedMethodName»(«attribute.stubDefaultClassVariableName»);
                    }
                «ENDIF»
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassTrySetMethodName»(«attribute.getTypeName(fInterface.model)» value) {
                if (!«attribute.stubDefaultClassValidateMethodName»(value))
                    return false;

                const bool valueChanged = («attribute.stubDefaultClassVariableName» != value);
                «attribute.stubDefaultClassVariableName» = std::move(value);
                return valueChanged;
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassValidateMethodName»(const «attribute.getTypeName(fInterface.model)»& value) {
                return true;
            }

            «IF !attribute.readonly»
                void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, «attribute.getTypeName(fInterface.model)» value) {
                    «attribute.stubDefaultClassSetMethodName»(value);
                }

                void «fInterface.stubDefaultClassName»::«attribute.stubRemoteEventClassChangedMethodName»() {
                    // No operation in default
                }

                void «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassChangedMethodName»() {
                    defaultStub_->«attribute.stubRemoteEventClassChangedMethodName»();
                }

                bool «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassSetMethodName»(«attribute.getTypeName(fInterface.model)» value) {
                    return defaultStub_->«attribute.stubDefaultClassTrySetMethodName»(std::move(value));
                }

                bool «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassSetMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, «attribute.getTypeName(fInterface.model)» value) {
                    return «attribute.stubRemoteEventClassSetMethodName»(value);
                }
            «ENDIF»

        «ENDFOR»

        «FOR method : fInterface.methods»
            «FTypeGenerator::generateComments(method, false)»
            void «fInterface.stubDefaultClassName»::«method.elementName»(«method.generateStubSignature») {
                // Call old style methods in default 
                «method.elementName»(«method.generateArgumentsToStubCompatibility»);
            }
            void «fInterface.stubDefaultClassName»::«method.elementName»(«method.generateStubSignatureOldStyle») {
                // No operation in default
            }

        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            «FTypeGenerator::generateComments(broadcast, false)»
            «IF !broadcast.selective.nullOrEmpty»
                void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireSelectiveMethodName»(«generateSendSelectiveSignatur(broadcast, fInterface, false)») {
                    stubAdapter_->«broadcast.stubAdapterClassSendSelectiveMethodName»(«broadcast.outArgs.map[elementName].join(', ')»«IF(!broadcast.outArgs.empty)», «ENDIF»receivers);
                }
                void «fInterface.stubDefaultClassName»::«broadcast.subscriptionChangedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId, const CommonAPI::SelectiveBroadcastSubscriptionEvent event) {
                    // No operation in default
                }
                bool «fInterface.stubDefaultClassName»::«broadcast.subscriptionRequestedMethodName»(const std::shared_ptr<CommonAPI::ClientId> clientId) {
                    // Accept in default
                    return true;
                }
                std::shared_ptr<CommonAPI::ClientIdList> const «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassSubscribersMethodName»() {
                    return(stubAdapter_->«broadcast.stubAdapterClassSubscribersMethodName»());
                }

            «ELSE»
                void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + getTypeName(fInterface.model) + '& ' + elementName].join(', ')») {
                    stubAdapter_->«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map[elementName].join(', ')»);
                }
            «ENDIF»
        «ENDFOR»

        «FOR managed : fInterface.managedInterfaces»
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedAutoName»(std::shared_ptr<«managed.stubClassName»> stub) {
                autoInstanceCounter_++;
                std::stringstream ss;
                ss << stubAdapter_->getInstanceId() << ".i" << autoInstanceCounter_;
                std::string instance = ss.str();
                return stubAdapter_->«managed.stubRegisterManagedName»(stub, instance);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubRegisterManagedMethodImpl» {
                return stubAdapter_->«managed.stubRegisterManagedName»(stub, instance);
            }
            bool «fInterface.stubDefaultClassName»::«managed.stubDeregisterManagedName»(const std::string& instance) {
                return stubAdapter_->«managed.stubDeregisterManagedName»(instance);
            }
            std::set<std::string>& «fInterface.stubDefaultClassName»::«managed.stubManagedSetGetterName»() {
                return stubAdapter_->«managed.stubManagedSetGetterName»();
            }
        «ENDFOR»

        «fInterface.stubDefaultClassName»::RemoteEventHandler::RemoteEventHandler(«fInterface.stubDefaultClassName»* defaultStub):
                defaultStub_(defaultStub) {
        }

        «fInterface.model.generateNamespaceEndDeclaration»
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
