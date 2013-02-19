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

class FInterfaceGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FTypeCommonAreaGenerator
    @Inject private extension FrancaGeneratorExtensions

	def generate(FInterface fInterface, IFileSystemAccess fileSystemAccess, DeploymentInterfacePropertyAccessor deploymentAccessor) {
        fileSystemAccess.generateFile(fInterface.headerPath, fInterface.generateHeader(deploymentAccessor))

        if (fInterface.hasSourceFile)
            fileSystemAccess.generateFile(fInterface.sourcePath, fInterface.generateSource)
	}

    def private generateHeader(FInterface fInterface, DeploymentInterfacePropertyAccessor deploymentAccessor) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fInterface.defineName»_H_
        #define «fInterface.defineName»_H_

        «FOR requiredHeaderFile : fInterface.requiredHeaderFiles.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «fInterface.model.generateNamespaceBeginDeclaration»

        class «fInterface.name»«IF fInterface.base != null»: public «fInterface.base.getRelativeNameReference(fInterface)»«ENDIF» {
         public:
            virtual ~«fInterface.name»() { }

            static inline const char* getInterfaceId();
            static inline CommonAPI::Version getInterfaceVersion();
            «fInterface.generateFTypeDeclarations(deploymentAccessor)»
        };
        
        const char* «fInterface.name»::getInterfaceId() {
            return "«fInterface.fullyQualifiedName»";
        }

        CommonAPI::Version «fInterface.name»::getInterfaceVersion() {
            return CommonAPI::Version(«fInterface.version.major», «fInterface.version.minor»);
        }

        «FOR type : fInterface.types»
           «type.generateFTypeInlineImplementation(fInterface, deploymentAccessor)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
        
        namespace CommonAPI {
        	«fInterface.generateTypeWriters(deploymentAccessor)»

        	«fInterface.generateVariantComparators»
        }


        namespace std {
            «fInterface.generateHashers(deploymentAccessor)»
        }

        #endif // «fInterface.defineName»_H_
    '''
    


    def private generateSource(FInterface fInterface) '''
        «generateCommonApiLicenseHeader»
        #include "«fInterface.headerFile»"

        «fInterface.model.generateNamespaceBeginDeclaration»

        «FOR type : fInterface.types»
           «type.generateFTypeImplementation(fInterface)»
        «ENDFOR»

        «fInterface.model.generateNamespaceEndDeclaration»
    '''

    def private getRequiredHeaderFiles(FInterface fInterface) {
        val headerSet = newHashSet('CommonAPI/types.h')
        fInterface.types.forEach[addRequiredHeaders(headerSet)]
        headerSet.remove(fInterface.headerPath)
        return headerSet
    }

    def private hasSourceFile(FInterface fInterface) {
        val hasTypeWithImplementation = fInterface.types.exists[hasImplementation]
        return hasTypeWithImplementation
    }
}