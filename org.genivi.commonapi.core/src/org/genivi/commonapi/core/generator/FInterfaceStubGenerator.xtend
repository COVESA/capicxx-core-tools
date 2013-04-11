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

class FInterfaceStubGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

	def generateStub(FInterface fInterface, IFileSystemAccess fileSystemAccess) {
        fileSystemAccess.generateFile(fInterface.stubHeaderPath, fInterface.generateStubHeader)
        fileSystemAccess.generateFile(fInterface.stubDefaultHeaderPath, fInterface.generateStubDefaultHeader)
        fileSystemAccess.generateFile(fInterface.stubDefaultSourcePath, fInterface.generateStubDefaultSource)
	}

    def private generateStubHeader(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_STUB_H_
        #define «fInterface.defineName»_STUB_H_

        «fInterface.generateRequiredTypeIncludes»
        #include "«fInterface.name».h"
        #include <CommonAPI/Stub.h>

        «fInterface.model.generateNamespaceBeginDeclaration»

        /**
         * Receives messages from remote and handles all dispatching of deserialized calls
         * to a stub for the service «fInterface.name». Also provides means to send broadcasts
         * and attribute-changed-notifications of observable attributes as defined by this service.
         * An application developer should not need to bother with this class.
         */
        class «fInterface.stubAdapterClassName»: virtual public CommonAPI::StubAdapter, public «fInterface.name» {
         public:
            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    ///Notifies all remote listeners about a change of value of the attribute «attribute.name».
                    virtual void «attribute.stubAdapterClassFireChangedMethodName»(const «attribute.type.getNameReference(fInterface.model)»& «attribute.name») = 0;
                «ENDIF»
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                /**
                 * Sends a broadcast event for «broadcast.name». Should not be called directly.
                 * Instead, the "fire<broadcastName>Event" methods of the stub should be used.
                 */
                virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')») = 0;
            «ENDFOR»        
        };


        /**
         * Defines the necessary callbacks to handle remote set events related to the attributes
         * defined in the IDL description for «fInterface.name».
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
                /// Verification callback for remote set requests on the attribute «attribute.name».
                virtual bool «attribute.stubRemoteEventClassSetMethodName»(«attribute.type.getNameReference(fInterface.model)» «attribute.name») = 0;
                /// Action callback for remote set requests on the attribute «attribute.name».
                virtual void «attribute.stubRemoteEventClassChangedMethodName»() = 0;

            «ENDFOR»
        };


        /**
         * Defines the interface that must be implemented by any class that should provide
         * the service «fInterface.name» to remote clients.
         * This class and the one above are the ones an application developer needs to have
         * a look at if he wants to implement a service.
         */
        class «fInterface.stubClassName» : public CommonAPI::Stub<«fInterface.stubAdapterClassName» , «fInterface.stubRemoteEventClassName»> {
         public:
            virtual ~«fInterface.stubClassName»() { }

            «FOR attribute : fInterface.attributes»
                /// Provides getter access to the attribute «attribute.name».
                virtual const «attribute.type.getNameReference(fInterface.model)»& «attribute.stubClassGetMethodName»() = 0;
            «ENDFOR»

            «FOR method: fInterface.methods»
                /// This is the method that will be called on remote calls on the method «method.name».
                virtual void «method.name»(«method.generateStubSignature») = 0;
            «ENDFOR»
            
            «FOR broadcast : fInterface.broadcasts»
                /// Sends a broadcast event for «broadcast.name».
                virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')») = 0;
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_STUB_H_
	'''

    def private generateStubDefaultHeader(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_STUB_DEFAULT_H_
        #define «fInterface.defineName»_STUB_DEFAULT_H_

        #include <«fInterface.stubHeaderPath»>

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
        class «fInterface.stubDefaultClassName» : public «fInterface.stubClassName» {
         public:
            «fInterface.stubDefaultClassName»();

            «fInterface.stubRemoteEventClassName»* initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»>& stubAdapter);

            «FOR attribute : fInterface.attributes»
                virtual const «attribute.type.getNameReference(fInterface.model)»& «attribute.stubClassGetMethodName»();
                virtual void «attribute.stubDefaultClassSetMethodName»(«attribute.type.getNameReference(fInterface.model)» value);

            «ENDFOR»

            «FOR method: fInterface.methods»
                virtual void «method.name»(«method.generateStubSignature»);

            «ENDFOR»
            
            «FOR broadcast : fInterface.broadcasts»
                virtual void «broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')»);
            «ENDFOR»

         protected:
            «FOR attribute : fInterface.attributes»
                virtual void «attribute.stubRemoteEventClassChangedMethodName»();
                virtual bool «attribute.stubDefaultClassTrySetMethodName»(«attribute.type.getNameReference(fInterface.model)» value);
                virtual bool «attribute.stubDefaultClassValidateMethodName»(const «attribute.type.getNameReference(fInterface.model)»& value);

            «ENDFOR»
            
         private:
            class RemoteEventHandler: public «fInterface.stubRemoteEventClassName» {
             public:
                RemoteEventHandler(«fInterface.stubDefaultClassName»* defaultStub);

                «FOR attribute : fInterface.attributes»
                    virtual bool «attribute.stubRemoteEventClassSetMethodName»(«attribute.type.getNameReference(fInterface.model)» value);
                    virtual void «attribute.stubRemoteEventClassChangedMethodName»();

                «ENDFOR»

             private:
                «fInterface.stubDefaultClassName»* defaultStub_;
            };

            RemoteEventHandler remoteEventHandler_;
            std::shared_ptr<«fInterface.stubAdapterClassName»> stubAdapter_;

            «FOR attribute : fInterface.attributes»
                «attribute.type.getNameReference(fInterface.model)» «attribute.stubDefaultClassVariableName»;
            «ENDFOR»            
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_STUB_DEFAULT_H_
    '''

    def private generateStubDefaultSource(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #include <«fInterface.stubDefaultHeaderPath»>

        «fInterface.model.generateNamespaceBeginDeclaration»

        «fInterface.stubDefaultClassName»::«fInterface.stubDefaultClassName»():
                remoteEventHandler_(this) {
        }

        «fInterface.stubRemoteEventClassName»* «fInterface.stubDefaultClassName»::initStubAdapter(const std::shared_ptr<«fInterface.stubAdapterClassName»>& stubAdapter) {
            stubAdapter_ = stubAdapter;
            return &remoteEventHandler_;
        }

        «FOR attribute : fInterface.attributes»
            const «attribute.type.getNameReference(fInterface.model)»& «fInterface.stubDefaultClassName»::«attribute.stubClassGetMethodName»() {
                return «attribute.stubDefaultClassVariableName»;
            }

            void «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassSetMethodName»(«attribute.type.getNameReference(fInterface.model)» value) {
                «IF attribute.isObservable»const bool valueChanged = «ENDIF»«attribute.stubDefaultClassTrySetMethodName»(std::move(value));
                «IF attribute.isObservable»
                    if (valueChanged)
                        stubAdapter_->«attribute.stubAdapterClassFireChangedMethodName»(«attribute.stubDefaultClassVariableName»);
                «ENDIF»   
            }

            void «fInterface.stubDefaultClassName»::«attribute.stubRemoteEventClassChangedMethodName»() {
                // No operation in default
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassTrySetMethodName»(«attribute.type.getNameReference(fInterface.model)» value) {
                if (!«attribute.stubDefaultClassValidateMethodName»(value))
                    return false;

                const bool valueChanged = («attribute.stubDefaultClassVariableName» != value);
                «attribute.stubDefaultClassVariableName» = std::move(value);
                return valueChanged;
            }

            bool «fInterface.stubDefaultClassName»::«attribute.stubDefaultClassValidateMethodName»(const «attribute.type.getNameReference(fInterface.model)»& value) {
                return true;
            }

            bool «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassSetMethodName»(«attribute.type.getNameReference(fInterface.model)» value) {
                return defaultStub_->«attribute.stubDefaultClassTrySetMethodName»(std::move(value));
            }

            void «fInterface.stubDefaultClassName»::RemoteEventHandler::«attribute.stubRemoteEventClassChangedMethodName»() {
                defaultStub_->«attribute.stubRemoteEventClassChangedMethodName»();
            }

        «ENDFOR»

        «FOR method : fInterface.methods»
            void «fInterface.stubDefaultClassName»::«method.name»(«method.generateStubSignature») {
                // No operation in default
            }

        «ENDFOR»
        
        «FOR broadcast : fInterface.broadcasts»
            void «fInterface.stubDefaultClassName»::«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map['const ' + type.getNameReference(fInterface.model) + '& ' + name].join(', ')») {
                stubAdapter_->«broadcast.stubAdapterClassFireEventMethodName»(«broadcast.outArgs.map[name].join(', ')»);
            }
        «ENDFOR»

        «fInterface.stubDefaultClassName»::RemoteEventHandler::RemoteEventHandler(«fInterface.stubDefaultClassName»* defaultStub):
                defaultStub_(defaultStub) {
        }

        «fInterface.model.generateNamespaceEndDeclaration»
    '''

    def private getStubDefaultClassSetMethodName(FAttribute fAttribute) {
        'set' + fAttribute.name.toFirstUpper + 'Attribute'
    }

    def private getStubDefaultClassTrySetMethodName(FAttribute fAttribute) {
        'trySet' + fAttribute.name.toFirstUpper + 'Attribute'
    }

    def private getStubDefaultClassValidateMethodName(FAttribute fAttribute) {
        'validate' + fAttribute.name.toFirstUpper + 'AttributeRequestedValue'
    }
    
    def private getStubDefaultClassVariableName(FAttribute fAttribute) {
        fAttribute.name.toFirstLower + 'AttributeValue_'
    }
}