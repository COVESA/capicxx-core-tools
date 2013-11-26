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
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor

import java.util.HashSet
import org.eclipse.core.resources.IResource

class FInterfaceProxyGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

    def generateProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess, DeploymentInterfacePropertyAccessor deploymentAccessor, IResource modelid) {
        fileSystemAccess.generateFile(fInterface.proxyBaseHeaderPath, fInterface.generateProxyBaseHeader(deploymentAccessor, modelid))
        fileSystemAccess.generateFile(fInterface.proxyHeaderPath, fInterface.generateProxyHeader(modelid))
    }

    def private generateProxyBaseHeader(FInterface fInterface, DeploymentInterfacePropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_PROXY_BASE_H_
        #define «fInterface.defineName»_PROXY_BASE_H_

        #include "«fInterface.headerFile»"
        «IF fInterface.base != null»
            #include "«fInterface.base.proxyBaseHeaderFile»"
        «ENDIF»

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

        «IF !fInterface.managedInterfaces.empty»
            #include <CommonAPI/ProxyManager.h>
        «ENDIF»
        «IF fInterface.hasAttributes»
            #include <CommonAPI/Attribute.h>
        «ENDIF»
        «IF fInterface.hasBroadcasts»
            #include <CommonAPI/Event.h>
            «IF fInterface.hasSelectiveBroadcasts»
                #include <CommonAPI/SelectiveEvent.h>
            «ENDIF»
        «ENDIF»
        #include <CommonAPI/Proxy.h>
        «IF !fInterface.methods.empty»
            #include <functional>
            #include <future>
        «ENDIF»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.proxyBaseClassName»: virtual public «IF fInterface.base != null»«fInterface.base.proxyBaseClassName»«ELSE»CommonAPI::Proxy«ENDIF» {
         public:
            «FOR attribute : fInterface.attributes»
                typedef CommonAPI::«attribute.commonApiBaseClassname»<«attribute.getTypeName(fInterface.model)»> «attribute.className»;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                «IF broadcast.isSelective»
                    typedef CommonAPI::SelectiveEvent<«broadcast.outArgs.map[getTypeName(fInterface.model)].join(', ')»> «broadcast.className»;
                «ELSE»
                    typedef CommonAPI::Event<«broadcast.outArgs.map[getTypeName(fInterface.model)].join(', ')»> «broadcast.className»;
                «ENDIF»
            «ENDFOR»

            «fInterface.generateAsyncCallbackTypedefs»

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                virtual «attribute.generateGetMethodDefinition» = 0;
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                virtual «broadcast.generateGetMethodDefinition» = 0;
            «ENDFOR»

            «FOR method : fInterface.methods»
                «FTypeGenerator::generateComments(method, false)»
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
            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»() = 0;
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»

        #endif // «fInterface.defineName»_PROXY_BASE_H_
    '''

    def private generateProxyHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_PROXY_H_
        #define «fInterface.defineName»_PROXY_H_

        #include "«fInterface.proxyBaseHeaderFile»"

        «IF fInterface.base != null»
            #include "«fInterface.base.proxyHeaderFile»"
        «ENDIF»

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
        class «fInterface.proxyClassName»: virtual public «fInterface.elementName», virtual public «fInterface.proxyBaseClassName»
        «IF fInterface.base != null»
            , virtual public «fInterface.base.proxyClassName»<_AttributeExtensions...>
        «ENDIF»
        , public _AttributeExtensions... {
        public:
            «fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate);
            ~«fInterface.proxyClassName»();

            typedef «fInterface.getRelativeNameReference(fInterface)» InterfaceType;

            «IF fInterface.base != null»
                inline static const char* getInterfaceId() {
                    return(«fInterface.elementName»::getInterfaceId());
                }
            «ENDIF»

            «FOR attribute : fInterface.attributes»
                «FTypeGenerator::generateComments(attribute, false)»
                /**
                 * Returns the wrapper class that provides access to the attribute «attribute.elementName».
                 */
                virtual «attribute.generateGetMethodDefinition» {
                    return delegate_->get«attribute.className»();
                }
            «ENDFOR»

            «FOR broadcast : fInterface.broadcasts»
                «FTypeGenerator::generateComments(broadcast, false)»
                /**
                 * Returns the wrapper class that provides access to the broadcast «broadcast.elementName».
                 */
                virtual «broadcast.generateGetMethodDefinition» {
                    return delegate_->get«broadcast.className»();
                }
            «ENDFOR»

            «FOR method : fInterface.methods»
                /**
                «FTypeGenerator::generateComments(method, true)»
                 * Calls «method.elementName» with «IF method.isFireAndForget»Fire&Forget«ELSE»synchronous«ENDIF» semantics.
                 * 
                «IF !method.inArgs.empty»* All const parameters are input parameters to this method.«ENDIF»
                «IF !method.outArgs.empty»* All non-const parameters will be filled with the returned values.«ENDIF»
                 * The CallStatus will be filled when the method returns and indicate either
                 * "SUCCESS" or which type of error has occurred. In case of an error, ONLY the CallStatus
                 * will be set.
                 */
                virtual «method.generateDefinition»;
                «IF !method.isFireAndForget»
                    /**
                     * Calls «method.elementName» with asynchronous semantics.
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

            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»();
            «ENDFOR»

            /**
             * Returns the CommonAPI address of the remote partner this proxy communicates with.
             */
            virtual std::string getAddress() const;

            /**
             * Returns the domain of the remote partner this proxy communicates with.
             */
            virtual const std::string& getDomain() const;

            /** 
             * Returns the service ID of the remote partner this proxy communicates with.
             */
            virtual const std::string& getServiceId() const;

            /**
             * Returns the instance ID of the remote partner this proxy communicates with.
             */
            virtual const std::string& getInstanceId() const;

            /**
             * Returns true if the remote partner for this proxy is currently known to be available.
             */
            virtual bool isAvailable() const;

            /**
             * Returns true if the remote partner for this proxy is available.
             */
            virtual bool isAvailableBlocking() const;

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
                «IF fInterface.base != null»
                «fInterface.base.proxyClassName»<_AttributeExtensions...>(delegate),
                «ENDIF»
                delegate_(std::dynamic_pointer_cast<«fInterface.proxyBaseClassName»>(delegate)),
                _AttributeExtensions(*(std::dynamic_pointer_cast<«fInterface.proxyBaseClassName»>(delegate)))... {
        }

        template <typename ... _AttributeExtensions>
        «fInterface.proxyClassName»<_AttributeExtensions...>::~«fInterface.proxyClassName»() {
        }

        «FOR method : fInterface.methods»
            «FTypeGenerator::generateComments(method, false)»
            template <typename ... _AttributeExtensions>
            «method.generateDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                delegate_->«method.elementName»(«method.generateSyncVariableList»);
            }
            «IF !method.isFireAndForget»

                template <typename ... _AttributeExtensions>
                «method.generateAsyncDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>')» {
                    return delegate_->«method.elementName»Async(«method.generateASyncVariableList»);
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
        bool «fInterface.proxyClassName»<_AttributeExtensions...>::isAvailableBlocking() const {
            return delegate_->isAvailableBlocking();
        }

        template <typename ... _AttributeExtensions>
        CommonAPI::ProxyStatusEvent& «fInterface.proxyClassName»<_AttributeExtensions...>::getProxyStatusEvent() {
            return delegate_->getProxyStatusEvent();
        }

        template <typename ... _AttributeExtensions>
        CommonAPI::InterfaceVersionAttribute& «fInterface.proxyClassName»<_AttributeExtensions...>::getInterfaceVersionAttribute() {
            return delegate_->getInterfaceVersionAttribute();
        }

        «FOR managed : fInterface.managedInterfaces»
            template <typename ... _AttributeExtensions>
            CommonAPI::ProxyManager& «fInterface.proxyClassName»<_AttributeExtensions...>::«managed.proxyManagerGetterName»() {
                return delegate_->«managed.proxyManagerGetterName»();
            }
        «ENDFOR»

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
        «FTypeGenerator::generateComments(fAttribute, false)»
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

    def private generateAsyncCallbackTypedefs(FInterface fInterface) '''
        «var callbackDefinitions = new HashSet<String>()»
        «FOR fMethod : fInterface.methods»
            «IF !fMethod.isFireAndForget»
                «val definitionSignature = fMethod.generateASyncTypedefSignature + fMethod.asyncCallbackClassName»
                «IF !callbackDefinitions.contains(definitionSignature)»
                    «IF fMethod.needsMangling»
                        /*
                         * This method has overloaded output parameters!
                         *
                         * A hash value was generated out of the names and types of these output
                         * parameters to distinguish the callback typedefs. Be careful in changing the
                         * type definitions in Franca if you actively use these typedefs in your code.
                         */
                    «ENDIF»
                    typedef std::function<void(«fMethod.generateASyncTypedefSignature»)> «fMethod.asyncCallbackClassName»;
                    «{callbackDefinitions.add(definitionSignature);""}»
                «ENDIF»
            «ENDIF»
        «ENDFOR»
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
        fInterface.elementName + 'Extensions'
    }

    def private getProxyClassName(FInterface fInterface) {
        fInterface.elementName + 'Proxy'
    }

    def private getExtensionClassName(FAttribute fAttribute) {
        return fAttribute.className + 'Extension'
    }

    def private generateSyncVariableList(FMethod fMethod) {
        val syncVariableList = new ArrayList(fMethod.inArgs.map[elementName])

        syncVariableList.add('callStatus')

        if (fMethod.hasError)
            syncVariableList.add('methodError')

        syncVariableList.addAll(fMethod.outArgs.map[elementName])

        return syncVariableList.join(', ')
    }

    def private generateASyncVariableList(FMethod fMethod) {
        var asyncVariableList = new ArrayList(fMethod.inArgs.map[elementName])
        asyncVariableList.add('callback')
        return asyncVariableList.join(', ')
    }
}
