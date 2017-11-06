package org.genivi.commonapi.core.generator

import java.util.HashSet
import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.PreferenceConstants

import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FStructType
import org.franca.core.franca.FTypeDef
import org.franca.core.franca.FArrayType
import org.franca.core.franca.FMapType
import org.franca.core.franca.FUnionType
import org.franca.core.franca.FEnumerationType

import org.franca.core.franca.FModelElement

class FInterfaceDumpGeneratorExtension {
    @Inject private extension FTypeGenerator
    @Inject private extension FrancaGeneratorExtensions
    @Inject private extension FInterfaceProxyGenerator

    var HashSet<FStructType> usedTypes;

    def generateDumper(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        usedTypes = new HashSet<FStructType>
        fileSystemAccess.generateFile(fInterface.serrializationHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGenerateSerrialiation(deploymentAccessor, modelid))
        fileSystemAccess.generateFile(fInterface.proxyDumpWrapperHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGenerateDumpClientWrapper(deploymentAccessor, modelid))
        fileSystemAccess.generateFile(fInterface.proxyDumpWriterHeaderPath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGenerateDumpClientWriter(deploymentAccessor, modelid))
    }

    def private getProxyDumpWrapperClassName(FInterface fInterface) {
        fInterface.proxyClassName + 'DumpWrapper'
    }

    def private getProxyDumpWriterClassName(FInterface fInterface) {
        fInterface.proxyClassName + 'DumpWriter'
    }

    def dispatch extGenerateTypeSerrialization(FTypeDef fTypeDef, FInterface fInterface) '''
        «extGenerateSerrializationMain(fTypeDef.actualType, fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FArrayType fArrayType, FInterface fInterface) '''
        «extGenerateSerrializationMain(fArrayType.elementType, fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FMapType fMap, FInterface fInterface) '''
        «extGenerateSerrializationMain(fMap.keyType, fInterface)»
        «extGenerateSerrializationMain(fMap.valueType, fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FEnumerationType fEnumerationType, FInterface fInterface) '''
        #ifndef «fEnumerationType.getDefineName(fInterface)»
        #define «fEnumerationType.getDefineName(fInterface)»
        ADAPT_NAMED_ATTRS_ADT(
        «(fEnumerationType as FModelElement).getElementName(fInterface, true)»,
        ("value_", value_)
        ,SIMPLE_ACCESS)
        #endif // «fEnumerationType.getDefineName(fInterface)»
    '''

    def dispatch extGenerateTypeSerrialization(FUnionType fUnionType, FInterface fInterface) '''
    '''

    def dispatch extGenerateTypeSerrialization(FStructType fStructType, FInterface fInterface) '''
        «IF usedTypes.add(fStructType)»
            «FOR fField : fStructType.elements»
                «extGenerateSerrializationMain(fField.type, fInterface)»
            «ENDFOR»

            // «fStructType.name»
            #ifndef «fStructType.getDefineName(fInterface)»
            #define «fStructType.getDefineName(fInterface)»
            ADAPT_NAMED_ATTRS_ADT(
            «(fStructType as FModelElement).getElementName(fInterface, true)»,
            «extGenerateFieldsSerrialization(fStructType, fInterface)» ,)
            #endif // «fStructType.getDefineName(fInterface)»
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

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
                «extGenerateSerrializationMain(attribute.type, fInterface)»
            «ENDIF»
        «ENDFOR»

        «FOR broadcast : fInterface.broadcasts»
            «FOR argument : broadcast.outArgs»
                «extGenerateSerrializationMain(argument.type, fInterface)»
            «ENDFOR»
        «ENDFOR»

        #endif // «fInterface.defineName»_SERRIALIZATION_HPP_
    '''

    def private extCommandTypeName(FInterface fInterface) '''
        SCommand«fInterface.name»'''

    def private extVersionTypeName(FInterface fInterface) '''
        SVersion«fInterface.name»'''

    def private extGenerateDumpClientWriter(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        #pragma once

        #include <fstream>

        #include <«fInterface.proxyDumpWrapperHeaderPath»>
        #include <«fInterface.serrializationHeaderPath»>

        struct «fInterface.extCommandTypeName()» {
            int64_t time;
            std::string name;
        };

        struct «fInterface.extVersionTypeName()» {
            uint32_t major;
            uint32_t minor;
        };

        ADAPT_NAMED_ATTRS_ADT(
        «fInterface.extCommandTypeName()»,
        ("time", time)
        ("name", name),
        SIMPLE_ACCESS)

        ADAPT_NAMED_ATTRS_ADT(
        «fInterface.extVersionTypeName()»,
        ("major", major)
        ("minor", minor),
        SIMPLE_ACCESS)

        class «fInterface.proxyDumpWriterClassName»
        {
        public:
            «fInterface.proxyDumpWriterClassName»(const std::string& file_name)
            {
                m_stream.open(file_name.c_str());
                if (!m_stream.is_open())
                {
                    throw std::runtime_error("Failed to open file '" + file_name + "'");
                }

                boost::property_tree::ptree child_ptree;

                «fInterface.extVersionTypeName()» version{«fInterface.version.major», «fInterface.version.minor»};
                JsonSerializer::Private::TPtreeSerializer<«fInterface.extVersionTypeName()»>::write(version, child_ptree);

                m_stream << "{\n\"" << "version" << "\": ";
                boost::property_tree::write_json(m_stream, child_ptree);
                m_stream << ",\"queries\": [\n";
            }

            ~«fInterface.proxyDumpWriterClassName»()
            {
                finishQuery(true);
                m_stream << "]\n}";
            }

            void beginQuery(const std::string& name)
            {
                if (!m_current_ptree.empty())
                {
                    finishQuery();
                }

                int64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                boost::property_tree::ptree child_ptree;
                JsonSerializer::Private::TPtreeSerializer<«fInterface.extCommandTypeName()»>::write({us, name}, child_ptree);

                m_current_ptree.add_child("declaration", child_ptree);
                child_ptree.clear();

                m_current_ptree.add_child("params", child_ptree);
            }

            template<class T>
            void adjustQuery(const T& var, const std::string& name)
            {
                boost::property_tree::ptree& data_ptree = m_current_ptree.get_child("params");
                boost::property_tree::ptree child_ptree;
                JsonSerializer::Private::TPtreeSerializer<T>::write(var, child_ptree);
                data_ptree.add_child(name, child_ptree);
            }

            void finishQuery(bool last = false)
            {
                boost::property_tree::write_json(m_stream, m_current_ptree);
                if (!last)
                {
                    m_stream << ",";
                }
                m_current_ptree.clear();
            }

        private:
            std::ofstream m_stream;
            boost::property_tree::ptree m_current_ptree;
        };
    '''

    def private extGenerateDumpClientWrapper(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        #pragma once
        #include <«fInterface.proxyHeaderPath»>
        #include <«fInterface.proxyDumpWriterHeaderPath»>

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        template <typename ..._AttributeExtensions>
        class «fInterface.proxyDumpWrapperClassName» : public «fInterface.proxyClassName»<_AttributeExtensions...>
        {
        public:
            «fInterface.proxyDumpWrapperClassName»(std::shared_ptr<CommonAPI::Proxy> delegate)
                : «fInterface.proxyClassName»<_AttributeExtensions...>(delegate)
                , m_writer("«fInterface.name»_dump.json")
            {
                std::cout << "Version : «fInterface.version.major».«fInterface.version.minor»" << std::endl;

                «FOR fAttribute : fInterface.attributes»
                    «fInterface.proxyClassName»<_AttributeExtensions...>::get«fAttribute.className»().
                        getChangedEvent().subscribe([this](const «fAttribute.getTypeName(fInterface, true)»& data)
                        {
                            // TODO: add mutex?
                            m_writer.beginQuery("«fAttribute.className»");
                            m_writer.adjustQuery(data, "«fAttribute.name»");
                        });
                «ENDFOR»
                «FOR broadcast : fInterface.broadcasts»
                    «fInterface.proxyClassName»<_AttributeExtensions...>::get«broadcast.className»().subscribe([this](
                        «var boolean first = true»
                        «FOR argument : broadcast.outArgs»
                            «IF !first»,«ENDIF»«IF first = false»«ENDIF» const «argument.getTypeName(argument, true)»& «argument.name»
                        «ENDFOR»
                        ) {
                            // TODO: add mutex?
                            m_writer.beginQuery("«broadcast.className»");
                            «FOR argument : broadcast.outArgs»
                                m_writer.adjustQuery(«argument.name», "«argument.name»");
                            «ENDFOR»
                        });
                «ENDFOR»
            }
        private:
            «fInterface.proxyDumpWriterClassName» m_writer;
        };

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»
    '''

}
