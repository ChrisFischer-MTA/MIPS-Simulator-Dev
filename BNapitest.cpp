#include <stdio.h>
#include <inttypes.h>
#include "binaryninjacore.h"
#include "binaryninjaapi.h"

using namespace BinaryNinja;
using namespace std;


#ifndef _WIN32
        #include <libgen.h>
        #include <dlfcn.h>
static string GetPluginsDirectory()
{
        Dl_info info;
        if (!dladdr((void*)BNGetBundledPluginDirectory, &info))
                return NULL;

        stringstream ss;
        ss << dirname((char*)info.dli_fname) << "/plugins/";
        return ss.str();
}
#else
static string GetPluginsDirectory()
{
        return "C:\\Program Files\\Vector35\\BinaryNinja\\plugins\\";
}
#endif

int main(int argc, char* argv[])
{
        if (argc != 2)
        {
                fprintf(stderr, "Expected input filename\n");
                return 1;
        }

        // In order to initiate the bundled plugins properly, the location
        // of where bundled plugins directory is must be set. Since
        // libbinaryninjacore is in the path get the path to it and use it to
        // determine the plugins directory
        SetBundledPluginDirectory(GetPluginsDirectory());
        InitPlugins();

        Ref<BinaryData> bd = new BinaryData(new FileMetadata(), argv[1]);
        Ref<BinaryView> bv;
        for (auto type : BinaryViewType::GetViewTypes())
        {
                if (type->IsTypeValidForData(bd) && type->GetName() != "Raw")
                {
                        bv = type->Create(bd);
                        break;
                }
        }

        if (!bv || bv->GetTypeName() == "Raw")
        {
                fprintf(stderr, "Input file does not appear to be an exectuable\n");
                return -1;
        }
        
        bv->UpdateAnalysisAndWait();

        printf("\nSections\n");
        printf("Name\t\t\tStart-Finish\t\t\tPerms\tNotes\n");

        for (auto& sec : bv->GetSections())
        {
                printf("%s\t\t\t0x%08lx-0x%08lx\t\t\t%d\t%s\n", 
                    sec->GetName().c_str(), sec->GetStart(), 
                    sec->GetStart()+sec->GetLength(), sec->GetSemantics(), 
                    sec->GetType().c_str())
        }

        printf("\nSegments\n");
        printf("Offset\t\tStart-Finish\t\tSize\tFlags\n");

        for (auto& seg : bv->GetSegments())
        {
                uint32_t temp = seg->GetFlags();
                bool readable = temp >> 2;
                bool writable = (temp >> 1) & 1;
                bool executable = temp & 1;
                
                uint64_t newBase = 0x0000000fffffffff;
                seg->GetStart((uint64_t) newBase);

                printf("0x%08lx\t0x%08lx-0x%08lx\t0x%08lx\t%d%d%d\n", 
                    seg->GetStart(), seg->GetStart(), 
                    seg->GetStart()+seg->GetLength(), 
                    seg->GetLength(), readable, writable, executable);
        }



        // Go through all functions in the binary
        for (auto& func : bv->GetAnalysisFunctionList())
        {
                // Get the name of the function and display it
                Ref<Symbol> sym = func->GetSymbol();
                if (sym)
                        printf("Function %s:\n", sym->GetFullName().c_str());
                else
                        printf("Function at 0x%" PRIx64 ":\n", func->GetStart());

                // Loop through all blocks in the function
                for (auto& block : func->GetBasicBlocks())
                {
                        // Set basic disasm settings
                        BinaryNinja::DisassemblySettings* settings;
                        BinaryNinja::DisassemblySettings tmp = DisassemblySettings();
                        settings = &tmp;

                        for (auto& distxt : block->GetDisassemblyText(settings))
                        {
                                // Address of instruction
                                printf("0x%08lx|", distxt.addr);

                                // Allocate bytes and buffer
                                size_t numBytesRead;
                                unsigned char* bytes = (unsigned char*) malloc(sizeof(char) * 4);

                                // Read bytes
                                numBytesRead = bv->Read(bytes, distxt.addr, 4);
                                printf("op->%x %x %x %x |",*(bytes+0),*(bytes+1),*(bytes+2),*(bytes+3));

                                // Print tokens
                                for (auto token = std::begin(distxt.tokens); token != std::end(distxt.tokens); ++token)
                                {
                                        printf("%s", token->text.c_str());
                                }
                                printf("\n");
                        }
                }

                printf("\n");
        }


        // Shutting down is required to allow for clean exit of the core
        BNShutdown();

        return 0;
}
