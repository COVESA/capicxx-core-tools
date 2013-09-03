/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.genivi.commonapi.core.deployment.DeploymentInterfacePropertyAccessor
import java.util.Collection
import java.util.HashSet
import org.franca.core.franca.FMethod
import org.eclipse.core.resources.IResource

class FInterfaceGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FTypeCommonAreaGenerator
    @Inject private extension FrancaGeneratorExtensions

    def generate(FInterface fInterface, IFileSystemAccess fileSystemAccess, DeploymentInterfacePropertyAccessor deploymentAccessor, IResource modelid) {
        fileSystemAccess.generateFile(fInterface.headerPath, fInterface.generateHeader(deploymentAccessor, modelid))

        if (fInterface.hasSourceFile)
            fileSystemAccess.generateFile(fInterface.sourcePath, fInterface.generateSource(modelid))
    }

    def private generateHeader(FInterface fInterface, DeploymentInterfacePropertyAccessor deploymentAccessor, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #ifndef «fInterface.defineName»_H_
        #define «fInterface.defineName»_H_

        «val libraryHeaders = new HashSet<String>»
        «val generatedHeaders = new HashSet<String>»
        «fInterface.getRequiredHeaderFiles(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #if !defined (COMMONAPI_INTERNAL_COMPILATION)
        #define COMMONAPI_INTERNAL_COMPILATION
        #endif

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        #undef COMMONAPI_INTERNAL_COMPILATION

        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.name»«IF fInterface.base != null»: public «fInterface.base.getRelativeNameReference(fInterface)»«ENDIF» {
         public:
            virtual ~«fInterface.name»() { }

            static inline const char* getInterfaceId();
            static inline CommonAPI::Version getInterfaceVersion();
            «fInterface.generateFTypeDeclarations(deploymentAccessor)»
        };

        const char* «fInterface.name»::getInterfaceId() {
            static const char* interfaceId = "«fInterface.fullyQualifiedName»";
            return interfaceId;
        }

        CommonAPI::Version «fInterface.name»::getInterfaceVersion() {
            return CommonAPI::Version(«fInterface.version.major», «fInterface.version.minor»);
        }

        «FOR type : fInterface.types»
           «FTypeGenerator::generateComments(type, false)»
           «type.generateFTypeInlineImplementation(fInterface, deploymentAccessor)»
        «ENDFOR»
        «FOR method : fInterface.methods.filter[errors != null]»
            «FTypeGenerator::generateComments(method, false)»
            «method.errors.generateInlineImplementation(method.errors.errorName, fInterface, fInterface.name, deploymentAccessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»

        namespace CommonAPI {
            «fInterface.generateTypeWriters(deploymentAccessor)»

            «fInterface.generateVariantComparators»
        }


        namespace std {
            //hashes for types
            «fInterface.generateHashers(deploymentAccessor)»
            
            //hashes for error types
            «FOR method : fInterface.methods.filter[errors != null]»
                «method.errors.generateHash(method.errors.errorName, fInterface, deploymentAccessor)»
            «ENDFOR»
        }

        #endif // «fInterface.defineName»_H_
    '''



    def private generateSource(FInterface fInterface, IResource modelid) '''
        «generateCommonApiLicenseHeader(fInterface, modelid)»
        «FTypeGenerator::generateComments(fInterface, false)»
        #include "«fInterface.headerFile»"

        «fInterface.model.generateNamespaceBeginDeclaration»

        «FOR type : fInterface.types»
           «FTypeGenerator::generateComments(type,false)»
           «type.generateFTypeImplementation(fInterface)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
    '''

    def void getRequiredHeaderFiles(FInterface fInterface, Collection<String> generatedHeaders, Collection<String> libraryHeaders) {
        libraryHeaders.add('CommonAPI/types.h')
        fInterface.types.forEach[addRequiredHeaders(generatedHeaders, libraryHeaders)]
        var Iterable<FMethod> errorMethods = fInterface.methods.filter[errors!=null]
        if(errorMethods.size!=0){
            libraryHeaders.addAll('CommonAPI/InputStream.h', 'CommonAPI/OutputStream.h')
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
        return hasTypeWithImplementation
    }
}