/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.ArrayList
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.PreferenceConstants
import org.genivi.commonapi.core.preferences.FPreferences

class FInterfaceProxyGenerator {
	@Inject extension FTypeGenerator
	@Inject extension FrancaGeneratorExtensions

    var boolean generateSyncCalls = true

    def generateProxy(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {
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
        «IF fInterface.base !== null»
            #include <«fInterface.base.proxyBaseHeaderPath»>
        «ENDIF»

        «val generatedHeaders = new HashSet<String>»
        «val libraryHeaders = new HashSet<String>»
        «fInterface.generateRequiredTypeIncludes(generatedHeaders, libraryHeaders, false)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «startInternalCompilation»

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
        «ENDIF»
        #include <CommonAPI/Proxy.hpp>
        «IF !fInterface.methods.empty»
            #include <functional>
            #include <future>
        «ENDIF»

        «endInternalCompilation»

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.proxyBaseClassName»
            : virtual public «IF fInterface.base !== null»«fInterface.base.getTypeCollectionName(fInterface)»ProxyBase«ELSE»CommonAPI::Proxy«ENDIF» {
        public:
            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «val itsAttribute = itsElement»
                    typedef CommonAPI::«itsAttribute.commonApiBaseClassname»<«itsAttribute.getTypeName(fInterface, true)»> «itsAttribute.className»;
                «ELSEIF itsElement instanceof FBroadcast»
                    typedef CommonAPI::Event<
                        «itsElement.outArgs.map[getTypeName(fInterface, true)].join(', ')»
                    > «itsElement.className»;
                «ENDIF»
            «ENDFOR»

            «fInterface.generateAsyncCallbackTypedefs»

            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    virtual «itsElement.generateGetMethodDefinition» = 0;
                «ELSEIF itsElement instanceof FBroadcast»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    virtual «itsElement.generateGetMethodDefinition» = 0;
                «ELSEIF itsElement instanceof FMethod»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    «IF generateSyncCalls || itsElement.isFireAndForget»
                        «IF itsElement.isFireAndForget»
                            /**
                             * @invariant Fire And Forget
                             */
                        «ENDIF»
                        virtual «itsElement.generateDefinition(true)» = 0;
                    «ENDIF»
                    «IF !itsElement.isFireAndForget»
                        virtual «itsElement.generateAsyncDefinition(true)» = 0;
                    «ENDIF»
                «ENDIF»
            «ENDFOR»
            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»() = 0;
            «ENDFOR»

            virtual std::future<void> getCompletionFuture() = 0;
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

        «IF fInterface.base !== null»
            #include "«fInterface.base.proxyHeaderPath»"
        «ENDIF»

        «startInternalCompilation»
        
        «IF fInterface.hasAttributes»
            #include <CommonAPI/AttributeExtension.hpp>
            #include <CommonAPI/Factory.hpp>
        «ENDIF»

        «endInternalCompilation»

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ... TAttributeExtensions>
        class «fInterface.proxyClassName»
            : virtual public «fInterface.elementName»,
              virtual public «fInterface.proxyBaseClassName»,«IF fInterface.base !== null»
              public «fInterface.base.getTypeCollectionName(fInterface)»Proxy<TAttributeExtensions...>,«ENDIF»
              virtual public TAttributeExtensions... {
        public:
            «fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate);
            ~«fInterface.proxyClassName»();

            typedef «fInterface.versionPrefix»«fInterface.model.generateCppNamespace»«fInterface.getRelativeNameReference(fInterface)» InterfaceType;

            «IF fInterface.base !== null»
                inline static const char* getInterface() {
                    return(«fInterface.elementName»::getInterface());
                }
            «ENDIF»

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

            virtual std::future<void> getCompletionFuture();

            «FOR itsElement : fInterface.elements»
                «IF itsElement instanceof FAttribute»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    /**
                     * Returns the wrapper class that provides access to the attribute «itsElement.elementName».
                     */
                    virtual «itsElement.generateGetMethodDefinition» {
                        return delegate_->get«itsElement.className»();
                    }
                «ELSEIF itsElement instanceof FBroadcast»
                    «FTypeGenerator::generateComments(itsElement, false)»
                    /**
                     * Returns the wrapper class that provides access to the broadcast «itsElement.elementName».
                     */
                    virtual «itsElement.generateGetMethodDefinition» {
                        return delegate_->get«itsElement.className»();
                    }
                «ELSEIF itsElement instanceof FMethod»
                    «IF generateSyncCalls || itsElement.isFireAndForget»
                        /**
                        «FTypeGenerator::generateComments(itsElement, true)»
                         * Calls «itsElement.elementName» with «IF itsElement.isFireAndForget»Fire&Forget«ELSE»synchronous«ENDIF» semantics.
                         *
                        «IF !itsElement.inArgs.empty» * All const parameters are input parameters to this method.«ENDIF»
                        «IF !itsElement.outArgs.empty» * All non-const parameters will be filled with the returned values.«ENDIF»
                         * The CallStatus will be filled when the method returns and indicate either
                         * "SUCCESS" or which type of error has occurred. In case of an error, ONLY the CallStatus
                         * will be set.
                         */
                        virtual «itsElement.generateDefinition(true)»;
                    «ENDIF»
                    «IF !itsElement.isFireAndForget»
                        /**
                         * Calls «itsElement.elementName» with asynchronous semantics.
                         *
                         * The provided callback will be called when the reply to this call arrives or
                         * an error occurs during the call. The CallStatus will indicate either "SUCCESS"
                         * or which type of error has occurred. In case of any error, ONLY the CallStatus
                         * will have a defined value.
                         * The std::future returned by this method will be fulfilled at arrival of the reply.
                         * It will provide the same value for CallStatus as will be handed to the callback.
                         */
                        virtual «itsElement.generateAsyncDefinition(true)»;
                    «ENDIF»
                «ENDIF»
            «ENDFOR»

            «FOR managed : fInterface.managedInterfaces»
                virtual CommonAPI::ProxyManager& «managed.proxyManagerGetterName»();
            «ENDFOR»


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
        template <typename ... TAttributeExtensions>
        «fInterface.proxyClassName»<TAttributeExtensions...>::«fInterface.proxyClassName»(std::shared_ptr<CommonAPI::Proxy> delegate):
                «IF fInterface.base !== null»
                «fInterface.base.getFullName()»Proxy<TAttributeExtensions...>(delegate),
                «ENDIF»
                TAttributeExtensions(*(std::dynamic_pointer_cast< «fInterface.proxyBaseClassName»>(delegate)))...,
                delegate_(std::dynamic_pointer_cast< «fInterface.proxyBaseClassName»>(delegate)) {
        }

        template <typename ... TAttributeExtensions>
        «fInterface.proxyClassName»<TAttributeExtensions...>::~«fInterface.proxyClassName»() {
        }

        «FOR itsElement : fInterface.elements»
            «IF itsElement instanceof FMethod»
                «FTypeGenerator::generateComments(itsElement, false)»
                «IF generateSyncCalls || itsElement.isFireAndForget»
                template <typename ... TAttributeExtensions>
                «itsElement.generateDefinitionWithin(fInterface.proxyClassName + '<TAttributeExtensions...>', false)» {
                    «FOR arg : itsElement.inArgs»
                        «IF !arg.array && arg.getType.supportsValidation»
                            if (!_«arg.elementName».validate()) {
                                _internalCallStatus = CommonAPI::CallStatus::INVALID_VALUE;
                                return;
                            }
                        «ENDIF»
                    «ENDFOR»
                    delegate_->«itsElement.elementName»(«itsElement.generateSyncVariableList»);
                }
                «ENDIF»
                «IF !itsElement.isFireAndForget»

                    template <typename ... TAttributeExtensions>
                    «itsElement.generateAsyncDefinitionWithin(fInterface.proxyClassName + '<TAttributeExtensions...>', false)» {
                        «FOR arg : itsElement.inArgs»
                            «IF !arg.array && arg.getType.supportsValidation»
                                if (!_«arg.elementName».validate()) {
                                    «itsElement.generateDummyArgumentDefinitions»
                                     «val callbackArguments = itsElement.generateDummyArgumentList»
                                    _callback(CommonAPI::CallStatus::INVALID_VALUE«IF callbackArguments != ""», «callbackArguments»«ENDIF»);
                                    std::promise<CommonAPI::CallStatus> promise;
                                    promise.set_value(CommonAPI::CallStatus::INVALID_VALUE);
                                    return promise.get_future();
                                }
                            «ENDIF»
                        «ENDFOR»
                        return delegate_->«itsElement.elementName»Async(«itsElement.generateASyncVariableList»);
                    }
                «ENDIF»
            «ENDIF»
        «ENDFOR»

        template <typename ... TAttributeExtensions>
        const CommonAPI::Address &«fInterface.proxyClassName»<TAttributeExtensions...>::getAddress() const {
            return delegate_->getAddress();
        }

        template <typename ... TAttributeExtensions>
        bool «fInterface.proxyClassName»<TAttributeExtensions...>::isAvailable() const {
            return delegate_->isAvailable();
        }

        template <typename ... TAttributeExtensions>
        bool «fInterface.proxyClassName»<TAttributeExtensions...>::isAvailableBlocking() const {
            return delegate_->isAvailableBlocking();
        }

        template <typename ... TAttributeExtensions>
        CommonAPI::ProxyStatusEvent& «fInterface.proxyClassName»<TAttributeExtensions...>::getProxyStatusEvent() {
            return delegate_->getProxyStatusEvent();
        }

        template <typename ... TAttributeExtensions>
        CommonAPI::InterfaceVersionAttribute& «fInterface.proxyClassName»<TAttributeExtensions...>::getInterfaceVersionAttribute() {
            return delegate_->getInterfaceVersionAttribute();
        }

        «FOR managed : fInterface.managedInterfaces»
            template <typename ... TAttributeExtensions>
            CommonAPI::ProxyManager& «fInterface.proxyClassName»<TAttributeExtensions...>::«managed.proxyManagerGetterName»() {
                return delegate_->«managed.proxyManagerGetterName»();
            }
        «ENDFOR»

        template <typename ... TAttributeExtensions>
        std::future<void> «fInterface.proxyClassName»<TAttributeExtensions...>::getCompletionFuture() {
            return delegate_->getCompletionFuture();
        }

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
}
