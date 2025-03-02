#undef NDEBUG
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <cwchar>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <vector>
#include <artec/sdk/capturing/IScanner.h>
#include <artec/sdk/capturing/IArrayScannerId.h>
#include <artec/sdk/capturing/IFrameProcessor.h>
#include <artec/sdk/capturing/IFrame.h>
#include <artec/sdk/base/BaseSdkDefines.h>
#include <artec/sdk/base/Log.h>
#include <artec/sdk/base/io/ObjIO.h>
#include <artec/sdk/base/IFrameMesh.h>
#include <artec/sdk/base/TArrayRef.h>
namespace asdk {
    using namespace artec::sdk::base;
    using namespace artec::sdk::capturing;
};
using asdk::TRef;
using asdk::TArrayRef;

// #define OUTPUT_DIR "C:\\Users\\iit_c\\Desktop\\socket\\socket_server\\app\\"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Error: Missing output path argument. Please insert the output folder first, followed by the desired amount of scans." << std::endl;
        return 1;
    }

    // Access the command line arguments
    /*
    for (int i = 1; i < argc; i++) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
    }
    */

    // Use the last command line argument as the output directory
    std::string outputDir = std::string(argv[argc - 2]) + "\\";
    int nFrames = std::stoi(argv[argc - 1]);
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "Frames number: " << nFrames << std::endl;

    // Create the output directory if it doesn't exist
    std::filesystem::create_directory(outputDir);

    std::vector<TRef<asdk::IFrameMesh>> meshes;
    asdk::setOutputLevel( asdk::VerboseLevel_Trace );
    asdk::ErrorCode ec = asdk::ErrorCode_OK;
    TRef<asdk::IArrayScannerId> scannersList;
    std::wcout << L"Enumerating scanners... ";
    ec = asdk::enumerateScanners( &scannersList );
    if( ec != asdk::ErrorCode_OK )
    {
        std::wcout << L"failed" << std::endl;
        return 1;
    }
    std::wcout << L"done" << std::endl;
    int scanner_count = scannersList->getSize();
    if( scanner_count == 0 )
    {
        std::wcout << L"No scanners found" << std::endl;
        return 3;
    }
    const asdk::ScannerId* idArray = scannersList->getPointer();
    const asdk::ScannerId& defaultScanner = idArray[0]; // just take the first available scanner
    std::wcout 
        << L"Connecting to " << asdk::getScannerTypeName( defaultScanner.type ) 
        << L" scanner " << defaultScanner.serial << L"... "
    ;
    TRef<asdk::IScanner> scanner;
    ec = asdk::createScanner( &scanner, &defaultScanner );
    if( ec != asdk::ErrorCode_OK )
        {
            std::wcout << L"failed" << std::endl;
            return 2;
        }
    std::wcout << L"done" << std::endl;
        
    for (int y = 0; y < nFrames; y++) {   
        std::wcout << L"Capturing frame " << y << "... ";
        TRef<asdk::IFrame> frame;
        TRef<asdk::IFrameProcessor> processor;
        ec = scanner->createFrameProcessor(&processor);
        
        if (ec == asdk::ErrorCode_OK) {
            frame = NULL;
            ec = scanner->capture(&frame, true); // with texture
            
            if (ec == asdk::ErrorCode_OK) {
                TRef<asdk::IFrameMesh> mesh;
                ec = processor->reconstructAndTexturizeMesh(&mesh, frame);
                
                if (ec == asdk::ErrorCode_OK) {
                    std::wcout << L"done" << std::endl;
                    meshes.push_back(mesh); // Store the captured mesh
                } else {
                    std::wcout << L"failed to reconstruct mesh" << std::endl;
                }
            } else {
                std::wcout << L"failed to capture frame" << std::endl;
            }
        } else {
            std::wcout << L"failed to create frame processor" << std::endl;
        }
    }

    // Save all captured meshes at once
    for (size_t i = 0; i < meshes.size(); ++i) {
        // Get current time in milliseconds
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::wstringstream wss;
        wss << outputDir.c_str() << L"frame_" << ms << L".obj"; // Use timestamp as filename
        std::wstring filename = wss.str();
        
        ec = asdk::io::Obj::save(filename.c_str(), meshes[i]);
        
        if (ec == asdk::ErrorCode_OK) {
            std::wcout << L"Captured mesh " << ms << L" saved to disk." << std::endl;
        } else {
            std::wcout << L"Failed to save mesh " << ms << L"." << std::endl;
        }
    }

    return 0;
}
