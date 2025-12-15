// Nova Package Manager - npm-compatible package manager
#include <iostream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244 4245 4267 4310 4324 4456 4458 4459 4624)
#endif

#include "nova/PackageManager/PackageManager.h"
#include "nova/Version.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace nova;

void printUsage() {
    std::string versionBanner = "Nova Package Manager " NOVA_VERSION;
    size_t totalWidth = 63;
    size_t versionWidth = versionBanner.length();
    size_t padding = (totalWidth - versionWidth) / 2;
    std::string centeredVersion = std::string(padding, ' ') + versionBanner;

    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║ )" << centeredVersion << R"( ║
║              Fast Package Manager with Caching                ║
╚═══════════════════════════════════════════════════════════════╝

Usage: nnpm <command> [options] [packages...]

Commands:
  init [ts]           Initialize a new project (ts = with TypeScript)
  install, i <pkg>    Install package(s)
  update, u [pkg]     Update package(s) to latest version
  uninstall, un <pkg> Remove a package
  ci                  Clean install from lockfile
  link [pkg]          Link package globally or to project
  list, ls            Show dependency tree
  outdated            Check for outdated packages
  login [registry]    Log in to registry
  logout [registry]   Log out from registry
  pack                Create tarball for publishing
  publish             Publish package to registry
  config              Show current configuration
  test [pattern]      Run tests
  run <script>        Run script from package.json

Options:
  -S, --save          Save to dependencies (default)
  -D, --dev           Save to devDependencies
  -g, --global        Install/uninstall globally
  -p, -P, --peer      Save to peerDependencies
  -op, -Op, --optional Save to optionalDependencies
  --help, -h          Show this help
  --version, -v       Show version

Examples:
  # Initialize project
  nnpm init
  nnpm init ts          # With TypeScript

  # Install dependencies from package.json
  nnpm install
  nnpm i

  # Install a specific package
  nnpm install lodash
  nnpm i express

  # Install dev dependency
  nnpm install --save-dev typescript
  nnpm i -D @types/node

  # Install global package
  nnpm install -g typescript
  nnpm i -g nodemon

  # Update packages
  nnpm update
  nnpm u lodash

  # Remove a package
  nnpm uninstall lodash
  nnpm un express

  # Run tests
  nnpm test
  nnpm run test

  # Run custom script
  nnpm run dev
  nnpm run build

For more information: https://nova-lang.org/docs/pm
)" << std::endl;
}

void printVersion() {
    std::cout << NOVA_VERSION_STRING << std::endl;
    std::cout << "Copyright (c) 2025 Nova Lang Team" << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    if (command == "--version" || command == "-v") {
        printVersion();
        return 0;
    }

    // Normalize command aliases
    if (command == "i") command = "install";
    else if (command == "u") command = "update";
    else if (command == "un") command = "uninstall";
    else if (command == "ls") command = "list";

    // Parse options and arguments
    std::string inputFile;
    pm::DependencyType depType = pm::DependencyType::Production;
    bool isGlobal = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-S" || arg == "--save") {
            depType = pm::DependencyType::Production;
        } else if (arg == "-D" || arg == "--dev" || arg == "--save-dev") {
            depType = pm::DependencyType::Development;
        } else if (arg == "-g" || arg == "--global") {
            isGlobal = true;
            depType = pm::DependencyType::Global;
        } else if (arg == "-p" || arg == "-P" || arg == "--peer" || arg == "--save-peer") {
            depType = pm::DependencyType::Peer;
        } else if (arg == "-op" || arg == "-Op" || arg == "--optional" || arg == "--save-optional") {
            depType = pm::DependencyType::Optional;
        } else if (arg[0] != '-') {
            inputFile = arg;
        }
    }

    pm::PackageManager pmgr;

    // Handle init command
    if (command == "init") {
        bool withTypeScript = (inputFile == "ts" || inputFile == "typescript");
        bool success = pmgr.init(".", withTypeScript);
        return success ? 0 : 1;
    }

    // Handle install command
    if (command == "install") {
        pmgr.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        if (inputFile.empty()) {
            // Install all dependencies from package.json
            auto result = pmgr.install(".", true);
            if (!result.success) {
                for (const auto& err : result.errors) {
                    std::cerr << "[error] " << err << std::endl;
                }
                return 1;
            }
            return 0;
        }

        // Install specific package
        pm::InstallResult result;
        if (isGlobal) {
            result = pmgr.installGlobal(inputFile, "latest");
            if (result.success) {
                std::cout << "\n[nnpm] Installed " << inputFile << " globally" << std::endl;
            }
        } else {
            result = pmgr.add(inputFile, "latest", depType);
            if (result.success) {
                std::string typeStr = "dependencies";
                if (depType == pm::DependencyType::Development) typeStr = "devDependencies";
                else if (depType == pm::DependencyType::Peer) typeStr = "peerDependencies";
                else if (depType == pm::DependencyType::Optional) typeStr = "optionalDependencies";
                std::cout << "\n[nnpm] Added " << inputFile << " to " << typeStr << std::endl;
            }
        }
        return result.success ? 0 : 1;
    }

    // Handle update command
    if (command == "update") {
        pmgr.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        auto result = pmgr.update(inputFile, depType);
        if (result.success) {
            if (inputFile.empty()) {
                std::cout << "\n[nnpm] Updated all packages" << std::endl;
            } else {
                std::cout << "\n[nnpm] Updated " << inputFile << std::endl;
            }
        } else {
            for (const auto& err : result.errors) {
                std::cerr << "[error] " << err << std::endl;
            }
        }
        return result.success ? 0 : 1;
    }

    // Handle uninstall command
    if (command == "uninstall") {
        if (inputFile.empty()) {
            std::cerr << "[error] Please specify a package to uninstall" << std::endl;
            std::cerr << "Usage: npm uninstall <package-name> [-g]" << std::endl;
            return 1;
        }

        bool success;
        if (isGlobal) {
            success = pmgr.removeGlobal(inputFile);
            if (success) {
                std::cout << "[nnpm] Removed " << inputFile << " globally" << std::endl;
            }
        } else {
            success = pmgr.remove(inputFile, depType);
            if (success) {
                std::cout << "[nnpm] Removed " << inputFile << std::endl;
            }
        }
        return success ? 0 : 1;
    }

    // Handle ci command
    if (command == "ci") {
        pmgr.setProgressCallback([](const std::string& pkg, int current, int total, bool fromCache) {
            std::string status = fromCache ? " (cached)" : "";
            std::cout << "\r[" << current << "/" << total << "] " << pkg << status << "          " << std::flush;
        });

        auto result = pmgr.cleanInstall(inputFile.empty() ? "." : inputFile);
        if (!result.success) {
            for (const auto& err : result.errors) {
                std::cerr << "[error] " << err << std::endl;
            }
            return 1;
        }
        return 0;
    }

    // Handle link command
    if (command == "link") {
        if (inputFile.empty()) {
            bool success = pmgr.link(".");
            return success ? 0 : 1;
        } else {
            bool success = pmgr.linkPackage(inputFile);
            return success ? 0 : 1;
        }
    }

    // Handle list command
    if (command == "list") {
        pmgr.listDependencies(inputFile.empty() ? "." : inputFile);
        return 0;
    }

    // Handle outdated command
    if (command == "outdated") {
        pmgr.checkOutdated(inputFile.empty() ? "." : inputFile);
        return 0;
    }

    // Handle login command
    if (command == "login") {
        bool success = pmgr.login(inputFile);
        return success ? 0 : 1;
    }

    // Handle logout command
    if (command == "logout") {
        bool success = pmgr.logout(inputFile);
        return success ? 0 : 1;
    }

    // Handle pack command
    if (command == "pack") {
        std::string tarballPath = pmgr.pack(inputFile.empty() ? "." : inputFile);
        return tarballPath.empty() ? 1 : 0;
    }

    // Handle publish command
    if (command == "publish") {
        bool success = pmgr.publish(inputFile.empty() ? "." : inputFile);
        return success ? 0 : 1;
    }

    // Handle config command
    if (command == "config") {
        pmgr.loadNpmrc(inputFile.empty() ? "." : inputFile);
        const auto& config = pmgr.getNpmrcConfig();

        std::cout << "[nnpm] Configuration" << std::endl;
        std::cout << std::endl;
        std::cout << "Registry: " << config.registry << std::endl;

        if (!config.scopedRegistries.empty()) {
            std::cout << std::endl;
            std::cout << "Scoped Registries:" << std::endl;
            for (const auto& [scope, registry] : config.scopedRegistries) {
                std::cout << "  " << scope << " -> " << registry << std::endl;
            }
        }

        if (!config.authTokens.empty()) {
            std::cout << std::endl;
            std::cout << "Auth Tokens:" << std::endl;
            for (const auto& [registry, token] : config.authTokens) {
                std::string maskedToken = token.size() > 8 ?
                    token.substr(0, 4) + "..." + token.substr(token.size() - 4) : "****";
                std::cout << "  " << registry << " -> " << maskedToken << std::endl;
            }
        }

        std::cout << std::endl;
        std::cout << "Settings:" << std::endl;
        std::cout << "  save-exact: " << (config.saveExact ? "true" : "false") << std::endl;
        std::cout << "  strict-ssl: " << (config.strictSSL ? "true" : "false") << std::endl;
        std::cout << "  progress: " << (config.progress ? "true" : "false") << std::endl;
        std::cout << "  fetch-retries: " << config.fetchRetries << std::endl;
        std::cout << "  fetch-timeout: " << config.fetchTimeout << "ms" << std::endl;

        if (!config.proxy.empty()) {
            std::cout << "  proxy: " << config.proxy << std::endl;
        }
        if (!config.httpsProxy.empty()) {
            std::cout << "  https-proxy: " << config.httpsProxy << std::endl;
        }

        return 0;
    }

    // Handle test command
    if (command == "test") {
        int result = pmgr.runTests(".", inputFile);
        return result;
    }

    // Handle run command (run npm script)
    if (command == "run") {
        if (inputFile.empty()) {
            std::cerr << "[error] No script name specified" << std::endl;
            std::cerr << "Usage: npm run <script-name>" << std::endl;
            return 1;
        }

        int result = pmgr.runScript(inputFile, ".");
        return result;
    }

    std::cerr << "[error] Unknown command: " << command << std::endl;
    printUsage();
    return 1;
}
