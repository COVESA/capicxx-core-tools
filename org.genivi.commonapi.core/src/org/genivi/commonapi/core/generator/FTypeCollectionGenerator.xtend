/* Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.genivi.commonapi.core.generator

import java.util.Collection
import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FStructType
import org.franca.core.franca.FTypeCollection
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.FPreferences
import org.genivi.commonapi.core.preferences.PreferenceConstants

class FTypeCollectionGenerator {
	@Inject extension FTypeGenerator
	@Inject extension FTypeCommonAreaGenerator
	@Inject extension FrancaGeneratorExtensions

    def generate(FTypeCollection fTypeCollection,
                 IFileSystemAccess fileSystemAccess,
                 PropertyAccessor deploymentAccessor,
                 IResource modelid) {

        if(FPreferences::getInstance.getPreference(PreferenceConstants::P_GENERATE_CODE, "true").equals("true")) {
            fileSystemAccess.generateFile(fTypeCollection.headerPath, IFileSystemAccess.DEFAULT_OUTPUT, fTypeCollection.generateHeader(modelid, deploymentAccessor))

            if (fTypeCollection.hasSourceFile) {
                fileSystemAccess.generateFile(fTypeCollection.sourcePath, IFileSystemAccess.DEFAULT_OUTPUT, fTypeCollection.generateSource(modelid, deploymentAccessor))
            }
        }
        else {
            // feature: suppress code generation
            fileSystemAccess.generateFile(fTypeCollection.headerPath, IFileSystemAccess.DEFAULT_OUTPUT, PreferenceConstants::NO_CODE)

            if (fTypeCollection.hasSourceFile) {
                fileSystemAccess.generateFile(fTypeCollection.sourcePath, IFileSystemAccess.DEFAULT_OUTPUT, PreferenceConstants::NO_CODE)
            }
        }
    }

    def private generateHeader(FTypeCollection fTypeCollection, IResource modelid, PropertyAccessor deploymentAccessor) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fTypeCollection, false)»
        #ifndef «fTypeCollection.defineName»_HPP_
        #define «fTypeCollection.defineName»_HPP_

        «val libraryHeaders = new HashSet<String>»
        «val generatedHeaders = new HashSet<String>»
        «fTypeCollection.getRequiredHeaderFiles(generatedHeaders, libraryHeaders)»

        «FOR requiredHeaderFile : generatedHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «startInternalCompilation»

        «FOR requiredHeaderFile : libraryHeaders.sort»
            #include <«requiredHeaderFile»>
        «ENDFOR»

        «endInternalCompilation»

        «fTypeCollection.generateVersionNamespaceBegin»
        «fTypeCollection.model.generateNamespaceBeginDeclaration»

        struct «fTypeCollection.elementName» {
            «fTypeCollection.generateFTypeDeclarations(deploymentAccessor)»

        «FOR type : fTypeCollection.types»
            «type.generateFTypeInlineImplementation(type, deploymentAccessor)»
        «ENDFOR»
            «fTypeCollection.generateFConstDeclarations(deploymentAccessor)»

        static inline const char* getTypeCollectionName() {
            static const char* typeCollectionName = "«fTypeCollection.fullyQualifiedName»";
            return typeCollectionName;
        }

        «IF fTypeCollection.version !== null»
            inline CommonAPI::Version getTypeCollectionVersion() {
                return CommonAPI::Version(«fTypeCollection.version.major», «fTypeCollection.version.minor»);
            }
        «ENDIF»

        }; // struct «fTypeCollection.elementName»

        «fTypeCollection.model.generateNamespaceEndDeclaration»
        «fTypeCollection.generateVersionNamespaceEnd»

        namespace CommonAPI {
            «fTypeCollection.generateVariantComparators»
        }


        namespace std {
            «fTypeCollection.generateHashers(deploymentAccessor)»
        }

        «fTypeCollection.generateMajorVersionNamespace»

        #endif // «fTypeCollection.defineName»_HPP_
    '''

    def private generateSource(FTypeCollection fTypeCollection, IResource modelid, PropertyAccessor _accessor) '''
        «generateCommonApiLicenseHeader()»
        «FTypeGenerator::generateComments(fTypeCollection, false)»
        #include "«fTypeCollection.headerFile»"

        «FOR fStructTypeHeaderPath : fTypeCollection.allDerivedFStructTypeHeaderPaths»
            #include <«fStructTypeHeaderPath»>
        «ENDFOR»

        «fTypeCollection.generateVersionNamespaceBegin»
        «fTypeCollection.model.generateNamespaceBeginDeclaration»

        «FOR type : fTypeCollection.types»
            «type.generateFTypeImplementation(fTypeCollection, _accessor)»
        «ENDFOR»

        «fTypeCollection.model.generateNamespaceEndDeclaration»
        «fTypeCollection.generateVersionNamespaceEnd»
    '''

    def void getRequiredHeaderFiles(FTypeCollection fInterface, Collection<String> generatedHeaders,
        Collection<String> libraryHeaders) {
        libraryHeaders.add('CommonAPI/Types.hpp')
        fInterface.types.forEach[addRequiredHeaders(generatedHeaders, libraryHeaders)]
        generatedHeaders.remove(fInterface.headerPath)
    }

    def private hasSourceFile(FTypeCollection fTypeCollection) {
        val hasTypeWithImplementation = fTypeCollection.types.exists[hasImplementation]
        return hasTypeWithImplementation
    }

    def private getAllDerivedFStructTypeHeaderPaths(FTypeCollection fTypeCollection) {
        return fTypeCollection.types.filter[it instanceof FStructType && (it as FStructType).hasPolymorphicBase].map[
            (it as FStructType).derivedFStructTypes].flatten.map[(eContainer as FTypeCollection).headerPath].toSet.filter[
            !fTypeCollection.headerPath.equals(it)]
    }
}
