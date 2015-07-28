/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.franca.core.franca.FMethod
import org.franca.core.franca.FVersion
import org.genivi.commonapi.core.deployment.PropertyAccessor

class FInterfaceGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FTypeCommonAreaGenerator
    @Inject private extension FrancaGeneratorExtensions

    def generateInterface(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {
        fileSystemAccess.generateFile(fInterface.headerPath, IFileSystemAccess.DEFAULT_OUTPUT, fInterface.generateHeader(modelid, deploymentAccessor))

        if (fInterface.hasSourceFile)
            fileSystemAccess.generateFile(fInterface.sourcePath, IFileSystemAccess.DEFAULT_OUTPUT, fInterface.generateSource(modelid, deploymentAccessor))
    }

    def private generateHeader(FInterface fInterface, IResource modelid, PropertyAccessor deploymentAccessor) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName.toUpperCase»_HPP_
        #define «fInterface.defineName.toUpperCase»_HPP_

        «val libraryHeaders = new HashSet<String>»
        «val generatedHeaders = new HashSet<String>»
        «fInterface.getRequiredHeaderFiles(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»
        
        «IF fInterface.base != null»
            #include <«fInterface.base.headerPath»>
        «ENDIF»

        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.elementName»«IF fInterface.base != null»
        	: virtual public «fInterface.base.getTypeCollectionName(fInterface)»«ENDIF» {
        public:
            virtual ~«fInterface.elementName»() { }

            static inline const char* getInterface();
            static inline CommonAPI::Version getInterfaceVersion();
            «fInterface.generateFTypeDeclarations(deploymentAccessor)»
        };

        const char* «fInterface.elementName»::getInterface() {
            return ("«fInterface.fullyQualifiedName»");
        }

        CommonAPI::Version «fInterface.elementName»::getInterfaceVersion() {
            «val FVersion itsVersion = fInterface.version»
            «IF itsVersion != null»
                return CommonAPI::Version(«itsVersion.major», «itsVersion.minor»);
            «ELSE»
                return CommonAPI::Version(0, 0);
            «ENDIF»
        }

        «FOR type : fInterface.types»
           «FTypeGenerator::generateComments(type, false)»
           «type.generateFTypeInlineImplementation(fInterface, deploymentAccessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        namespace CommonAPI {
            «fInterface.generateVariantComparators»
        }

        #endif // «fInterface.defineName.toUpperCase»_HPP_
    '''



    def private generateSource(FInterface fInterface, IResource modelid, PropertyAccessor _accessor) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #include "«fInterface.headerFile»"

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»
        
        «FOR method : fInterface.methodsWithError»
            «method.errors.generateFTypeImplementation(method.containingInterface, _accessor)»
        «ENDFOR»

        «FOR type : fInterface.types»
           «FTypeGenerator::generateComments(type,false)»
           «type.generateFTypeImplementation(fInterface, _accessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
    '''

    def void getRequiredHeaderFiles(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        libraryHeaders.add('CommonAPI/Types.hpp')
        if (!fInterface.methods.filter[errors != null].empty) {
            libraryHeaders.addAll('CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp')
        }
        if (!fInterface.managedInterfaces.empty) {
            generatedHeaders.add('set');
        }
        fInterface.types.forEach[addRequiredHeaders(generatedHeaders, libraryHeaders)]
        var Iterable<FMethod> errorMethods = fInterface.methods.filter[errors!=null]
        if(errorMethods.size!=0){
            libraryHeaders.addAll('CommonAPI/InputStream.hpp', 'CommonAPI/OutputStream.hpp')
            errorMethods.forEach[
                if (errors.base != null) {
                   errors.base.addRequiredHeaders(generatedHeaders, libraryHeaders)
                }
            ]
        }

        generatedHeaders.remove(fInterface.headerPath)
    }

    def private hasSourceFile(FInterface fInterface) {
        val hasTypeWithImplementation = fInterface.types.exists[hasImplementation]
        val hasMethodWithError = fInterface.methods.exists[hasError]
        return (hasTypeWithImplementation || hasMethodWithError) 
    }
}
