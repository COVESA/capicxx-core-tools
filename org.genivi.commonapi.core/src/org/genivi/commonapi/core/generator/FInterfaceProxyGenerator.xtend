/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.ArrayList
import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FEnumerationType
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import static com.google.common.base.Preconditions.*
import java.util.HashSet
import java.util.HashMap

class FInterfaceProxyGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

	def generateProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        fileSystemAccess.generateFile(fInterface.proxyBaseHeaderPath, fInterface.generateProxyBaseHeader(deploymentAccessor))
        fileSystemAccess.generateFile(fInterface.proxyHeaderPath, fInterface.generateProxyHeader)
	}

    def private generateProxyBaseHeader(FInterface fInterface, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_PROXY_BASE_H_
        #define «fInterface.defineName»_PROXY_BASE_H_
        
        #include "«fInterface.headerFile»"
        
        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»        
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders)»
        
        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»
        
        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif
        
        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»
       
        «IF fInterface.hasAttributes»
            #include <CommonAPI/Attribute.h>
        «ENDIF»
        «IF fInterface.hasBroadcasts»
            #include <CommonAPI/Event.h>
        «ENDIF»
        #include <CommonAPI/Proxy.h>
        «IF !fInterface.methods.empty»
            #include <functional>
            #include <future>
        «ENDIF»
        
        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.proxyBaseClassName»: virtual public CommonAPI::Proxy {
         public:
            «FOR attribute : fInterface.attributes»
                typedef CommonAPI::«attribute.commonApiBaseClassname»<«attribute.getTypeName(fInterface.model)»> «attribute.className»;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                typedef CommonAPI::Event<«broadcast.outArgs.map[getTypeName(fInterface.model)].join(', ')»> «broadcast.className»;
            «ENDFOR»
            «var callbackSet = new HashSet<String>()»
            «FOR method : fInterface.methods»
                «IF !method.isFireAndForget»
                    «val define = "typedef std::function<void(" + method.generateASyncTypedefSignature + ")> " + method.asyncCallbackClassName + ";"»
                    «IF !callbackSet.contains(define)»
                        «define»
                        «val ok = callbackSet.add(define)»
                    «ENDIF»
                «ENDIF» 
            «ENDFOR»

            «FOR attribute : fInterface.attributes»
                virtual «attribute.generateGetMethodDefinition» = 0;
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                virtual «broadcast.generateGetMethodDefinition» = 0;
            «ENDFOR»

            «FOR method : fInterface.methods»

                «IF method.isFireAndForget»
                    /**
                     * @invariant Fire And Forget
                     */
                «ENDIF»
                virtual «method.generateDefinition» = 0;
                «IF !method.isFireAndForget»
                    virtual «method.generateAsyncDefinition» = 0;
                «ENDIF»
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_PROXY_BASE_H_
    '''

    def private generateProxyHeader(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_PROXY_H_
        #define «fInterface.defineName»_PROXY_H_

        #include "«fInterface.proxyBaseHeaderFile»"
        
        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif
        
        «IF fInterface.hasAttributes»
            #include <CommonAPI/AttributeExtension.h>
            #include <CommonAPI/Factory.h>
        «ENDIF»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ... _AttributeExtensions>
        class «fInterface.proxyClassName»: virtual public «fInterface.name», virtual public «fInterface.proxyBaseClassName», public _AttributeExtensions... {
         public:
            «fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate);
            ~«fInterface.proxyClassName»();

            «FOR attribute : fInterface.attributes»
                /// Returns the wrapper class that provides access to the attribute «attribute.name».
                virtual «attribute.generateGetMethodDefinition» {
                    return delegate_->get«attribute.className»();
                }

            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                /// Returns the wrapper class that provides access to the broadcast «broadcast.name».
                virtual «broadcast.generateGetMethodDefinition» {
                    return delegate_->get«broadcast.className»();
                }

            «ENDFOR»

            «FOR method : fInterface.methods»

                /**
                 * Calls «method.name» with «IF method.isFireAndForget»Fire&Forget«ELSE»synchronous«ENDIF» semantics.
                 * 
«IF !method.inArgs.empty»     * All const parameters are input parameters to this method.«ENDIF»
«IF !method.outArgs.empty»     * All non-const parameters will be filled with the returned values.«ENDIF»
                 * The CallStatus will be filled when the method returns and indicate either
                 * "SUCCESS" or which type of error has occurred. In case of an error, ONLY the CallStatus
                 * will be set.
                 «IF !method.isFireAndForget»
                 * Synchronous calls are not supported (will block indefinitely) when mainloop integration is used.
                 «ENDIF»
                 */
                virtual «method.generateDefinition»;
                «IF !method.isFireAndForget»
                    /**
                     * Calls «method.name» with asynchronous semantics.
                     * 
                     * The provided callback will be called when the reply to this call arrives or
                     * an error occurs during the call. The CallStatus will indicate either "SUCCESS"
                     * or which type of error has occurred. In case of any error, ONLY the CallStatus
                     * will have a defined value.
                     * The std::future returned by this method will be fulfilled at arrival of the reply.
                     * It will provide the same value for CallStatus as will be handed to the callback.
                     */
                    virtual «method.generateAsyncDefinition»;
                «ENDIF»
            «ENDFOR»

            /// Returns the CommonAPI address of the remote partner this proxy communicates with.
            virtual std::string getAddress() const;

            /// Returns the domain of the remote partner this proxy communicates with.
            virtual const std::string& getDomain() const;

            /// Returns the service ID of the remote partner this proxy communicates with.
            virtual const std::string& getServiceId() const;

            /// Returns the instance ID of the remote partner this proxy communicates with.
            virtual const std::string& getInstanceId() const;

            /// Returns true if the remote partner for this proxy is available.
            virtual bool isAvailable() const;

            /**
             * Returns the wrapper class that is used to (de-)register for notifications about
             * the availability of the remote partner of this proxy.
             */
            virtual CommonAPI::ProxyStatusEvent& getProxyStatusEvent();

            /**
             * Returns the wrapper class that is used to access version information of the remote
             * partner of this proxy.
             */
            virtual CommonAPI::InterfaceVersionAttribute& getInterfaceVersionAttribute();

         private:
            std::shared_ptr<«fInterface.proxyBaseClassName»> delegate_;
        };

        «IF fInterface.hasAttributes»
            namespace «fInterface.extensionsSubnamespace» {
                «FOR attribute : fInterface.attributes»
                    «attribute.generateExtension(fInterface)»

                «ENDFOR»
            } // namespace «fInterface.extensionsSubnamespace»
        «ENDIF»

        //
        // «fInterface.proxyClassName» Implementation
        //
        template <typename ... _AttributeExtensions>
        «fInterface.proxyClassName»<_AttributeExtensions...>::«fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate):
                delegate_(std::dynamic_pointer_cast<«fInterface.proxyBaseClassName»>(delegate)),
                _AttributeExtensions(*(std::dynamic_pointer_cast<«fInterface.proxyBaseClassName»>(delegate)))... {
        }
        
        template <typename ... _AttributeExtensions>
        «fInterface.proxyClassName»<_AttributeExtensions...>::~«fInterface.proxyClassName»() {
        }

        «FOR method : fInterface.methods»
            template <typename ... _AttributeExtensions>
            «method.generateDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                delegate_->«method.name»(«method.generateSyncVariableList»);
            }
            «IF !method.isFireAndForget»

                template <typename ... _AttributeExtensions>
                «method.generateAsyncDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                    return delegate_->«method.name»Async(«method.generateASyncVariableList»);
                }
            «ENDIF»

        «ENDFOR»

        template <typename ... _AttributeExtensions>
        std::string «fInterface.proxyClassName»<_AttributeExtensions...>::getAddress() const {
            return delegate_->getAddress();
        }

        template <typename ... _AttributeExtensions>
        const std::string& «fInterface.proxyClassName»<_AttributeExtensions...>::getDomain() const {
            return delegate_->getDomain();
        }
        
        template <typename ... _AttributeExtensions>
        const std::string& «fInterface.proxyClassName»<_AttributeExtensions...>::getServiceId() const {
            return delegate_->getServiceId();
        }
        
        template <typename ... _AttributeExtensions>
        const std::string& «fInterface.proxyClassName»<_AttributeExtensions...>::getInstanceId() const {
            return delegate_->getInstanceId();
        }
        
        template <typename ... _AttributeExtensions>
        bool «fInterface.proxyClassName»<_AttributeExtensions...>::isAvailable() const {
            return delegate_->isAvailable();
        }
        
        template <typename ... _AttributeExtensions>
        CommonAPI::ProxyStatusEvent& «fInterface.proxyClassName»<_AttributeExtensions...>::getProxyStatusEvent() {
            return delegate_->getProxyStatusEvent();
        }
        
        template <typename ... _AttributeExtensions>
        CommonAPI::InterfaceVersionAttribute& «fInterface.proxyClassName»<_AttributeExtensions...>::getInterfaceVersionAttribute() {
            return delegate_->getInterfaceVersionAttribute();
        }

        «fInterface.model.generateNamespaceEndDeclaration»

        «IF fInterface.hasAttributes»
        namespace CommonAPI {
        template<template<typename > class _AttributeExtension>
        struct DefaultAttributeProxyFactoryHelper<«fInterface.model.generateCppNamespace»«fInterface.proxyClassName»,
            _AttributeExtension> {
            typedef typename «fInterface.model.generateCppNamespace»«fInterface.proxyClassName»<
                    «fInterface.attributes.map[fInterface.model.generateCppNamespace + fInterface.extensionsSubnamespace + '::' + extensionClassName + "<_AttributeExtension>"].join(", \n")»
            > class_t;
        };
        }
        «ENDIF»
        

        #endif // «fInterface.defineName»_PROXY_H_
    '''

    def private generateExtension(FAttribute fAttribute, FInterface fInterface) '''
        template <template <typename > class _ExtensionType>
        class «fAttribute.extensionClassName» {
         public:
            typedef _ExtensionType<«fInterface.proxyBaseClassName»::«fAttribute.className»> extension_type;

            static_assert(std::is_base_of<typename CommonAPI::AttributeExtension<«fInterface.proxyBaseClassName»::«fAttribute.className»>, extension_type>::value,
                          "Not CommonAPI Attribute Extension!");

            «fAttribute.extensionClassName»(«fInterface.proxyBaseClassName»& proxy): attributeExtension_(proxy.get«fAttribute.className»()) {
            }

            inline extension_type& get«fAttribute.extensionClassName»() {
                return attributeExtension_;
            }

         private:
            extension_type attributeExtension_;
        };
    '''

    def private getCommonApiBaseClassname(FAttribute fAttribute) {
        var baseClassname = 'Attribute'

        if (fAttribute.isReadonly)
            baseClassname = 'Readonly' + baseClassname

        if (fAttribute.isObservable)
            baseClassname = 'Observable' + baseClassname

        return baseClassname
    }

    def private getExtensionsSubnamespace(FInterface fInterface) {
        fInterface.name + 'Extensions'
    }

    def private getProxyClassName(FInterface fInterface) {
        fInterface.name + 'Proxy'
    }

    def private getExtensionClassName(FAttribute fAttribute) {
        return fAttribute.className + 'Extension'
    }

    def private getErrorName(FEnumerationType fMethodErrors) {
        checkArgument(fMethodErrors.eContainer instanceof FMethod, 'Not FMethod errors')
        (fMethodErrors.eContainer as FMethod).name + 'Error'
    }

    def private generateASyncTypedefSignature(FMethod fMethod) {
        var signature = 'const CommonAPI::CallStatus&'

        if (fMethod.hasError)
            signature = signature + ', const ' + fMethod.getErrorNameReference(fMethod.eContainer) + '&'

        if (!fMethod.outArgs.empty)
            signature = signature + ', ' + fMethod.outArgs.map['const ' + getTypeName(fMethod.model) + '&'].join(', ')

        return signature
    }

    def private generateSyncVariableList(FMethod fMethod) {
        val syncVariableList = new ArrayList(fMethod.inArgs.map[name])

        syncVariableList.add('callStatus')

        if (fMethod.hasError)
            syncVariableList.add('methodError')

        syncVariableList.addAll(fMethod.outArgs.map[name])

        return syncVariableList.join(', ')
    }

    def private generateASyncVariableList(FMethod fMethod) {
        var asyncVariableList = new ArrayList(fMethod.inArgs.map[name])
        asyncVariableList.add('callback')
        return asyncVariableList.join(', ')
    }
}
