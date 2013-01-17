/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import javax.inject.Inject
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FTypeCollection

class FTypeCollectionGenerator {
    @Inject private extension FTypeGenerator
    @Inject private extension FTypeCommonAreaGenerator
    @Inject private extension FrancaGeneratorExtensions

	def generate(FTypeCollection fTypeCollection, IFileSystemAccess fileSystemAccess) {
        fileSystemAccess.generateFile(fTypeCollection.headerPath, fTypeCollection.generateHeader)

        if (fTypeCollection.hasSourceFile)
            fileSystemAccess.generateFile(fTypeCollection.sourcePath, fTypeCollection.generateSource)
	}

    def private generateHeader(FTypeCollection fTypeCollection) '''
        «generateCommonApiLicenseHeader»
        #ifndef «fTypeCollection.defineName»_H_
        #define «fTypeCollection.defineName»_H_

        «FOR requiredHeaderFile : fTypeCollection.requiredHeaderFiles.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «fTypeCollection.model.generateNamespaceBeginDeclaration»

        namespace «fTypeCollection.name» {
        «FOR type : fTypeCollection.types»

            «type.generateFTypeDeclaration»
        «ENDFOR»

        «FOR type : fTypeCollection.types»
           «type.generateFTypeInlineImplementation(type)»
        «ENDFOR»

        
        static inline const char* getTypeCollectionName() {
            return "«fTypeCollection.fullyQualifiedName»";
        }

        «IF fTypeCollection.version != null»
            CommonAPI::Version getTypeCollectionVersion() {
                return CommonAPI::Version(«fTypeCollection.version.major», «fTypeCollection.version.minor»);
            }
        «ENDIF»

        } // namespace «fTypeCollection.name»

        «fTypeCollection.model.generateNamespaceEndDeclaration»
        
        namespace CommonAPI {
        	
        	«fTypeCollection.generateTypeWriters»
        	
        }

        #endif // «fTypeCollection.defineName»_H_
    '''

    def private generateSource(FTypeCollection fTypeCollection) '''
        «generateCommonApiLicenseHeader»
        #include "«fTypeCollection.headerFile»"

        «fTypeCollection.model.generateNamespaceBeginDeclaration»
        namespace «fTypeCollection.name» {

        «FOR type : fTypeCollection.types»
           «type.generateFTypeImplementation(type)»
        «ENDFOR»

        } // namespace «fTypeCollection.name»
        «fTypeCollection.model.generateNamespaceEndDeclaration»
    '''

    def private getRequiredHeaderFiles(FTypeCollection fTypeCollection) {
        val headerSet = newHashSet('CommonAPI/types.h')
        fTypeCollection.types.forEach[addRequiredHeaders(headerSet)]
        headerSet.remove(fTypeCollection.headerPath)
        return headerSet
    }

    def private hasSourceFile(FTypeCollection fTypeCollection) {
        val hasTypeWithImplementation = fTypeCollection.types.exists[hasImplementation]
        return hasTypeWithImplementation
    }
}