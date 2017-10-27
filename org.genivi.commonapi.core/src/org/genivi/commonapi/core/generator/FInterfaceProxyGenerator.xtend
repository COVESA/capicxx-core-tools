/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.ArrayList
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.PreferenceConstants
import org.genivi.commonapi.core.preferences.FPreferences

import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FStructType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FMapType
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FEnumerationType

import org.franca.core.franca.FModelElement
import org.franca.core.franca.FField

class FInterfaceProxyGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions

    var boolean generateSyncCalls = true
    var HashSet<FStructType> usedTypes;


    def generateProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        usedTypes = new HashSet<FStructType>
        fileSystemAccess.generateFile(fInterface.serrializationHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGenerateSerrialiation(deploymentAccessor, modelid))
        fileSystemAccess.generateFile(fInterface.proxyDumpWrapperHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGenerateDumpClientWrapper(deploymentAccessor, modelid))

        val String generateCode = FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true")
        if(generateCode.equals("true")) {
            generateSyncCalls = FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_SYNC_CALLS, "true").equals("true")
            fileSystemAccess.generateFile(fInterface.proxyBaseHeaderPath, PreferenceConstants.P_OUTPUT_PROXIES, fInterface.generateProxyBaseHeader(deploymentAccessor, modelid))
            fileSystemAccess.generateFile(fInterface.proxyHeaderPath, PreferenceConstants.P_OUTPUT_PROXIES, fInterface.generateProxyHeader(modelid))
        }
        else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fInterface.proxyBaseHeaderPath, PreferenceConstants.P_OUTPUT_PROXIES, PreferenceConstants::NO_CODE)
            fileSystemAccess.generateFile(fInterface.proxyHeaderPath, PreferenceConstants.P_OUTPUT_PROXIES, PreferenceConstants::NO_CODE)
        }
    }

    def private generateProxyBaseHeader(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_PROXY_BASE_HPP_
        #define «fInterface.defineName»_PROXY_BASE_HPP_

        #include <«fInterface.headerPath»>
        «IF fInterface.base != null»
            #include <«fInterface.base.proxyBaseHeaderPath»>
        «ENDIF»

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders, false)»

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
            #include <CommonAPI/ProxyManager.hpp>
        «ENDIF»
        «IF fInterface.hasAttributes»
            #include <CommonAPI/Attribute.hpp>
        «ENDIF»
        «IF fInterface.hasBroadcasts»
            #include <CommonAPI/Event.hpp>
            «IF fInterface.hasSelectiveBroadcasts»
                #include <CommonAPI/SelectiveEvent.hpp>
            «ENDIF»
        «ENDIF»
        #include <CommonAPI/Proxy.hpp>
        «IF !fInterface.methods.empty»
            #include <functional>
            #include <future>
        «ENDIF»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.proxyBaseClassName»
            : virtual public «IF fInterface.base != null»«fInterface.base.getTypeCollectionName(fInterface)»ProxyBase«ELSE»CommonAPI::Proxy«ENDIF» {
        public:
            «FOR attribute : fInterface.attributes»
                typedef CommonAPI::«attribute.commonApiBaseClassname»< «attribute.getTypeName(fInterface, true)»> «attribute.className»;
            «ENDFOR»
            «FOR broadcast : fInterface.broadcasts»
                «IF broadcast.isSelective»
                    typedef CommonAPI::SelectiveEvent< «broadcast.outArgs.map[getTypeName(fInterface, true)].join(', ')»> «broadcast.className»;
                «ELSE»
                    typedef CommonAPI::Event<
                        «broadcast.outArgs.map[getTypeName(fInterface, true)].join(', ')»
                    > «broadcast.className»;
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
                «IF generateSyncCalls || method.isFireAndForget»
                «IF method.isFireAndForget»
                    /**
                     * @invariant Fire And Forget
                     */
                «ENDIF»
                virtual «method.generateDefinition(true)» = 0;
                «ENDIF»
                «IF !method.isFireAndForget»
                    virtual «method.generateAsyncDefinition(true)» = 0;
                «ENDIF»
            «ENDFOR»
            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»() = 0;
            «ENDFOR»
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName»_PROXY_BASE_HPP_
    '''

    def private generateProxyHeader(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_PROXY_HPP_
        #define «fInterface.defineName»_PROXY_HPP_

        #include <«fInterface.proxyBaseHeaderPath»>

        «IF fInterface.base != null»
            #include "«fInterface.base.proxyHeaderPath»"
        «ENDIF»

        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif

        «IF fInterface.hasAttributes»
            #include <CommonAPI/AttributeExtension.hpp>
            #include <CommonAPI/Factory.hpp>
        «ENDIF»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ... _AttributeExtensions>
        class «fInterface.proxyClassName»
            : virtual public «fInterface.elementName»,
              virtual public «fInterface.proxyBaseClassName»,«IF fInterface.base != null»
              public «fInterface.base.getTypeCollectionName(fInterface)»Proxy<_AttributeExtensions...>,«ENDIF»
              virtual public _AttributeExtensions... {
        public:
            «fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate);
            ~«fInterface.proxyClassName»();

            typedef «fInterface.getRelativeNameReference(fInterface)» InterfaceType;

            «IF fInterface.base != null»
                inline static const char* getInterface() {
                    return(«fInterface.elementName»::getInterface());
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
                «IF generateSyncCalls || method.isFireAndForget»
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
                virtual «method.generateDefinition(true)»;
                «ENDIF»
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
                    virtual «method.generateAsyncDefinition(true)»;
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»();
            «ENDFOR»

            /**
             * Returns the CommonAPI address of the remote partner this proxy communicates with.
             */
            virtual const CommonAPI::Address &getAddress() const;

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
            std::shared_ptr< «fInterface.proxyBaseClassName»> delegate_;
        };

        typedef «fInterface.proxyClassName»<> «fInterface.proxyDefaultClassName»;

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
                «fInterface.base.getFullName()»Proxy<_AttributeExtensions...>(delegate),
                «ENDIF»
                _AttributeExtensions(*(std::dynamic_pointer_cast< «fInterface.proxyBaseClassName»>(delegate)))...,
                delegate_(std::dynamic_pointer_cast< «fInterface.proxyBaseClassName»>(delegate)) {
        }

        template <typename ... _AttributeExtensions>
        «fInterface.proxyClassName»<_AttributeExtensions...>::~«fInterface.proxyClassName»() {
        }

        «FOR method : fInterface.methods»
            «FTypeGenerator::generateComments(method, false)»
            «IF generateSyncCalls || method.isFireAndForget»
            template <typename ... _AttributeExtensions>
            «method.generateDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>', false)» {
                «FOR arg : method.inArgs»
                    «IF !arg.array && arg.getType.supportsValidation»
                        if (!_«arg.elementName».validate()) {
                            _internalCallStatus = CommonAPI::CallStatus::INVALID_VALUE;
                            return;
                        }
                    «ENDIF»
                «ENDFOR»
                delegate_->«method.elementName»(«method.generateSyncVariableList»);
            }
            «ENDIF»
            «IF !method.isFireAndForget»

                template <typename ... _AttributeExtensions>
                «method.generateAsyncDefinitionWithin(fInterface.proxyClassName + '<_AttributeExtensions...>', false)» {
                    «FOR arg : method.inArgs»
                        «IF !arg.array && arg.getType.supportsValidation»
                            if (!_«arg.elementName».validate()) {
                                «method.generateDummyArgumentDefinitions»
                                 «val callbackArguments = method.generateDummyArgumentList»
                                _callback(CommonAPI::CallStatus::INVALID_VALUE«IF callbackArguments != ""», «callbackArguments»«ENDIF»);
                                std::promise<CommonAPI::CallStatus> promise;
                                promise.set_value(CommonAPI::CallStatus::INVALID_VALUE);
                                return promise.get_future();
                            }
                        «ENDIF»
                    «ENDFOR»
                    return delegate_->«method.elementName»Async(«method.generateASyncVariableList»);
                }
            «ENDIF»
        «ENDFOR»

        template <typename ... _AttributeExtensions>
        const CommonAPI::Address &«fInterface.proxyClassName»<_AttributeExtensions...>::getAddress() const {
            return delegate_->getAddress();
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
        «fInterface.generateVersionNamespaceEnd»

        «IF fInterface.hasAttributes»
        namespace CommonAPI {
        template<template<typename > class _AttributeExtension>
        struct DefaultAttributeProxyHelper< «fInterface.versionPrefix»«fInterface.model.generateCppNamespace»«fInterface.proxyClassName»,
            _AttributeExtension> {
            typedef typename «fInterface.versionPrefix»«fInterface.model.generateCppNamespace»«fInterface.proxyClassName»<
                    «fInterface.attributes.map[fInterface.versionPrefix + fInterface.model.generateCppNamespace + fInterface.extensionsSubnamespace + '::' + extensionClassName + "<_AttributeExtension>"].join(", \n")»
            > class_t;
        };
        }
        «ENDIF»

        «fInterface.generateMajorVersionNamespace»

        #endif // «fInterface.defineName»_PROXY_HPP_
    '''

    def private generateExtension(FAttribute fAttribute, FInterface fInterface) '''
        «FTypeGenerator::generateComments(fAttribute, false)»
        template <template <typename > class _ExtensionType>
        class «fAttribute.extensionClassName» {
         public:
            typedef _ExtensionType< «fInterface.proxyBaseClassName»::«fAttribute.className»> extension_type;

            static_assert(std::is_base_of<typename CommonAPI::AttributeExtension< «fInterface.proxyBaseClassName»::«fAttribute.className»>, extension_type>::value,
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

    def private getProxyDefaultClassName(FInterface fInterface) {
        fInterface.proxyClassName + 'Default'
    }

    def private getProxyDumpWrapperClassName(FInterface fInterface) {
        fInterface.proxyClassName + 'DumpWrapper'
    }

    def private getExtensionClassName(FAttribute fAttribute) {
        return fAttribute.className + 'Extension'
    }

    def private generateSyncVariableList(FMethod fMethod) {
        val syncVariableList = new ArrayList(fMethod.inArgs.map['_' + elementName])

        syncVariableList.add('_internalCallStatus')

        if (fMethod.hasError)
            syncVariableList.add('_error')

        syncVariableList.addAll(fMethod.outArgs.map['_' + elementName])

        if (!fMethod.isFireAndForget)
            return syncVariableList.join(', ') + ", _info"
        else
            return syncVariableList.join(', ')
    }

    def private generateASyncVariableList(FMethod fMethod) {
        var asyncVariableList = new ArrayList(fMethod.inArgs.map['_' + elementName])
        asyncVariableList.add('_callback')
        if (fMethod.isFireAndForget) {
            return asyncVariableList.join(', ')
        } else {
            return asyncVariableList.join(', ') + ", _info"
        }
    }

    def dispatch extGenerateTypeSerrialization(FTypeDef fTypeDef, FInterface fInterface) '''
        «extGenerateSerrializationMain(fTypeDef.actualType, fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FArrayType fArrayType, FInterface fInterface) '''
    '''

    def dispatch extGenerateTypeSerrialization(FMapType fMap, FInterface fInterface) '''
        «extGenerateSerrializationMain(fMap.keyType, fInterface)»
        «extGenerateSerrializationMain(fMap.valueType, fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FEnumerationType fEnumerationType, FInterface fInterface) '''
    '''

    def dispatch extGenerateTypeSerrialization(FUnionType fUnionType, FInterface fInterface) '''
    '''

    def dispatch extGenerateTypeSerrialization(FStructType fStructType, FInterface fInterface) '''
        «IF usedTypes.add(fStructType)»
            «FOR fField : fStructType.elements»
                «extGenerateSerrializationMain(fField.type, fInterface)»
            «ENDFOR»

            // «fStructType.name»
            ADAPT_NAMED_ATTRS_ADT(
            «(fStructType as FModelElement).getElementName(fInterface, true)»,
            «extGenerateFieldsSerrialization(fStructType, fInterface)» ,)
        «ENDIF»
    '''

    def dispatch extGenerateFieldsSerrialization(FStructType fStructType, FInterface fInterface) '''
        «IF (fStructType.base != null)»
            «extGenerateFieldsSerrialization(fStructType.base, fInterface)»
        «ENDIF»
        «FOR fField : fStructType.elements»
            ("«fField.name»", «fField.name»)
        «ENDFOR»
    '''

    def dispatch extGenerateSerrializationMain(FTypeRef fTypeRef, FInterface fInterface) '''
        «IF fTypeRef.derived != null»
            «extGenerateTypeSerrialization(fTypeRef.derived, fInterface)»
        «ENDIF»
    '''

    def private extGenerateSerrialiation(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        #ifndef «fInterface.defineName»_SERRIALIZATION_HPP_
        #define «fInterface.defineName»_SERRIALIZATION_HPP_

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»

        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders, true)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
                «extGenerateSerrializationMain(attribute.type, fInterface)»
            «ENDIF»
        «ENDFOR»

        #endif // «fInterface.defineName»_SERRIALIZATION_HPP_
    '''

    def private extGenerateDumpClientWrapper(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        #pragma once
        #include <«fInterface.proxyHeaderPath»>

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ..._AttributeExtensions>
        class «fInterface.proxyDumpWrapperClassName» : public «fInterface.proxyClassName»<_AttributeExtensions...>
        {
        public:
            «fInterface.proxyDumpWrapperClassName»(std::shared_ptr<CommonAPI::Proxy> delegate)
                : «fInterface.proxyClassName»<_AttributeExtensions...>(delegate)
            {
            }
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
    '''

}
