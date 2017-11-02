package org.genivi.commonapi.core.generator

import javax.inject.Inject
import org.eclipse.core.resources.IResource
import org.eclipse.xtext.generator.IFileSystemAccess
import org.franca.core.franca.FInterface
import org.genivi.commonapi.core.deployment.PropertyAccessor
import org.genivi.commonapi.core.preferences.PreferenceConstants

class FInterfacePlaybackGeneratorExtension {
    @Inject private extension FrancaGeneratorExtensions

    def generatePlayback(FInterface fInterface, IFileSystemAccess fileSystemAccess, PropertyAccessor deploymentAccessor, IResource modelid) {

        fileSystemAccess.generateFile(fInterface.playbackSourcePath, PreferenceConstants.P_OUTPUT_SKELETON, fInterface.extGeneratePlayback(deploymentAccessor, modelid))
    }

    def private extGeneratePlayback(FInterface fInterface, PropertyAccessor deploymentAccessor, IResource modelid) '''
        #include <fstream>
        #include <iostream>
        #include <vector>
        #include <functional>

        #include "preprocessor/AdaptNamedAttrsAdt.hpp"
        #include "json_serializer/JsonSerializer.hpp"
        #include <«fInterface.serrializationHeaderPath»>
        #include <«fInterface.getStubHeaderPath»>

        «fInterface.generateVersionNamespaceBegin»
        «fInterface.model.generateNamespaceBeginDeclaration»

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
                class «attribute.name»Element;
            «ENDIF»
        «ENDFOR»


        class CVisitor
        {
        public:
            CVisitor(std::shared_ptr<«fInterface.getStubClassName»> transport)
                : m_transport(transport) {}

            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    void visit«attribute.name»(const «attribute.name»Element&);
                «ENDIF»
            «ENDFOR»

        private:
            std::shared_ptr<«fInterface.getStubClassName»> m_transport;
        };

        class IElement
        {
            virtual void visit(CVisitor& visitor) = 0;
        };

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
            class «attribute.name»Element : public IElement
            {
            public:
                «attribute.name»Element(const «attribute.getTypeName(fInterface, true)»& data)
                    : m_data(data){}

                void visit(CVisitor& visitor) override {
                    visitor.visit«attribute.name»(*this);
                }

                const «attribute.getTypeName(fInterface, true)»& getData() const {
                    return m_data;
                }

            private:
                «attribute.getTypeName(fInterface, true)» m_data;
            };

            «ENDIF»
        «ENDFOR»

        «FOR attribute : fInterface.attributes»
            «IF attribute.isObservable»
            void CVisitor::visit«attribute.name»(const «attribute.name»Element& data) {
                m_transport->fire«attribute.className»Changed(data.getData());
                std::cout << "«attribute.name»" << std::endl;
            }
            «ENDIF»
        «ENDFOR»

        struct SCall
        {
            std::string m_name;
            std::fstream::pos_type m_pos;
        };

        static const std::string s_time_key = "\"time\"";
        static const std::string s_name_key = "\"name\"";

        class JsonDumpReader
        {
        public:
            JsonDumpReader(const std::string& file_name);
            void getTimestamps(std::vector<int64_t>& res);
            void readItem(std::size_t ts_id, CVisitor& visitor);

        private:
            bool readKey(const std::string& src, const std::string& key, std::string& val);
            bool findBracket(const std::string& src, bool is_begin);

            void initFunctions();

            std::ifstream m_file;

            std::vector<int64_t> m_timestamps;
            std::vector<SCall> m_calls;

            std::map<std::string, std::function<void(CVisitor&, boost::property_tree::ptree pt)>> m_functions;
        };

        void JsonDumpReader::getTimestamps(std::vector<int64_t>& res)
        {
            res = m_timestamps;
        }

        JsonDumpReader::JsonDumpReader(const std::string &file_name)
        {
            m_file.open(file_name.c_str());

            if (!m_file.is_open())
            {
                throw std::runtime_error("failed to open file '" + file_name + "'");
            }

            // TODO : check version

            std::string line;
            while (std::getline(m_file, line))
            {
                std::string time_val;
                if (readKey(line, s_time_key, time_val))
                {
                    m_timestamps.push_back(std::atoll(time_val.c_str()));

                    std::getline(m_file, line);
                    std::string name_val;
                    if (!readKey(line, s_name_key, name_val))
                    {
                        throw std::runtime_error("something wrong with file structure");
                    }

                    // skip comma separator
                    std::getline(m_file, line);

                    // skip "params" keyword
                    std::getline(m_file, line);

                    // return to "{" symbol
                    m_calls.push_back({name_val, m_file.tellg() - std::fstream::pos_type(2)});
                }
            }

            m_file.close();
            m_file.open(file_name.c_str());

            initFunctions();
        }

        void JsonDumpReader::initFunctions()
        {
            m_functions = {
            «FOR attribute : fInterface.attributes»
                «IF attribute.isObservable»
                    {"«attribute.className»", [](CVisitor& visitor, boost::property_tree::ptree pt)
                        {
                            boost::property_tree::ptree tmp_pt = pt.get_child("«attribute.name»");

                            «attribute.getTypeName(fInterface, true)» data;
                            JsonSerializer::readFromPtree(tmp_pt, data);

                            «attribute.name»Element data_elem(data);
                            visitor.visit«attribute.name»(data_elem);
                        }
                    },
                «ENDIF»
            «ENDFOR»
            };
        }

        void JsonDumpReader::readItem(std::size_t ts_id, CVisitor& visitor)
        {
            SCall call = m_calls[ts_id];
            m_file.seekg(call.m_pos);

            std::string line;
            std::stringstream ss;
            int brackets = 0;
            do
            {
                std::getline(m_file, line);
                brackets += findBracket(line, true);
                brackets -= findBracket(line, false);
                ss << line << std::endl;
            }
            while (brackets);

            boost::property_tree::ptree pt;
            boost::property_tree::read_json(ss, pt);

            auto func = m_functions.find(call.m_name);
            if (func != m_functions.end())
            {
                func->second(visitor, pt);
            }
            // TODO: else throw
        }

        bool JsonDumpReader::readKey(const std::string& src, const std::string& key, std::string& val)
        {
            const std::size_t key_len = key.length();
            const std::size_t key_begin_pos = src.find(key.c_str(), key_len);
            if (key_begin_pos == std::string::npos)
            {
                return false;
            }

            const std::size_t key_end_pos = key_begin_pos + key_len;
            const std::size_t val_begin_pos = src.find("\"", key_end_pos) + 1;
            const std::size_t val_end_pos = src.find("\"", val_begin_pos);
            val = src.substr(val_begin_pos, val_end_pos - val_begin_pos);
            return true;
        }

        bool JsonDumpReader::findBracket(const std::string& src, bool is_begin)
        {
            const std::string to_find = is_begin ? "{" : "}";
            return src.find(to_find) != std::string::npos;
        }

        «fInterface.model.generateNamespaceEndDeclaration»
        «fInterface.generateVersionNamespaceEnd»

        #include <CommonAPI/CommonAPI.hpp>
        #include <timeService/CTimeClient.hpp>
        #include <«fInterface.stubDefaultHeaderPath»>

        int main(int argc, char** argv)
        {
            «fInterface.generateNamespaceUsage»

            // TODO: catch SIGINT

            if (argc < 3)
            {
                std::cout << "Format: filename serviceName\n";
                return 0;
            }

            std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
            std::shared_ptr<SensorsSourceStubDefault> service = std::make_shared<«fInterface.stubDefaultClassName»>();
            runtime->registerService("local", argv[2], service);
            std::cout << "Successfully Registered Service!" << std::endl;

            CVisitor visitor(service);
            JsonDumpReader reader(argv[1]);

            std::vector<int64_t> timestamps;
            reader.getTimestamps(timestamps);

            TimeService::CTimeClient time_client(timestamps);

            while (1)
            {
                std::size_t idx = static_cast<std::size_t>(time_client.waitForNexTimestamp());
                reader.readItem(idx, visitor);
            }
            return 0;
        }

    '''
}
