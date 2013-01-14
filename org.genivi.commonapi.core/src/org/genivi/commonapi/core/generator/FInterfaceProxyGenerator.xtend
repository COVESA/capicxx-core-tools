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

import static com.google.common.base.Preconditions.*

class FInterfaceProxyGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

	def generateProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess) {
        fileSystemAccess.generateFile(fInterface.proxyBaseHeaderPath, fInterface.generateProxyBaseHeader)
        fileSystemAccess.generateFile(fInterface.proxyHeaderPath, fInterface.generateProxyHeader)
	}

    def private generateProxyBaseHeader(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_PROXY_BASE_H_
        #define «fInterface.defineName»_PROXY_BASE_H_

        #include "«fInterface.headerFile»"
        «fInterface.generateRequiredTypeIncludes»
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

        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.proxyBaseClassName»: virtual public CommonAPI::Proxy {
         public:
            «FOR method : fInterface.methods.filter[errors != null]»
                «method.errors.generateDeclaration(method.errors.errorName)»

            «ENDFOR»
            «FOR attribute : fInterface.attributes»
                typedef CommonAPI::«attribute.commonApiBaseClassname»<«attribute.type.getNameReference(fInterface.model)»> «attribute.className»;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                typedef CommonAPI::Event<«broadcast.outArgs.map[type.getNameReference(fInterface.model)].join(', ')»> «broadcast.className»;
            «ENDFOR»
            «FOR method : fInterface.methods»
                «IF !method.isFireAndForget»
                    typedef std::function<void(«method.generateASyncTypedefSignature»)> «method.asyncCallbackClassName»;
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

        «FOR method : fInterface.methods.filter[errors != null]»
            «method.errors.generateInlineImplementation(method.errors.errorName, fInterface, fInterface.proxyBaseClassName)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_PROXY_BASE_H_
    '''

    def private generateProxyHeader(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_PROXY_H_
        #define «fInterface.defineName»_PROXY_H_

        #include "«fInterface.proxyBaseHeaderFile»"
        «IF fInterface.hasAttributes»
            #include <CommonAPI/AttributeExtension.h>
            #include <CommonAPI/Factory.h>
        «ENDIF»

        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ... _AttributeExtensions>
        class «fInterface.proxyClassName»: virtual public «fInterface.name», virtual public «fInterface.proxyBaseClassName», public _AttributeExtensions... {
         public:
            «fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate);
            ~«fInterface.proxyClassName»();

            «FOR attribute : fInterface.attributes»
                virtual «attribute.generateGetMethodDefinition»;
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                virtual «broadcast.generateGetMethodDefinition»;
            «ENDFOR»

            «FOR method : fInterface.methods»

                virtual «method.generateDefinition»;
                «IF !method.isFireAndForget»
                    virtual «method.generateAsyncDefinition»;
                «ENDIF»
            «ENDFOR»

            virtual std::string getAddress() const;
            virtual const std::string& getDomain() const;
            virtual const std::string& getServiceId() const;
            virtual const std::string& getInstanceId() const;
            virtual bool isAvailable() const;
            virtual CommonAPI::ProxyStatusEvent& getProxyStatusEvent();
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

        «FOR attribute : fInterface.attributes»
            template <typename ... _AttributeExtensions>
            typename «attribute.generateGetMethodDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                return delegate_->get«attribute.className»();
            }

        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            template <typename ... _AttributeExtensions>
            typename «broadcast.generateGetMethodDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                return delegate_->get«broadcast.className»();
            }

        «ENDFOR»

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
            signature = signature + ', ' + fMethod.outArgs.map['const ' + type.getNameReference(fMethod.model) + '&'].join(', ')

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
