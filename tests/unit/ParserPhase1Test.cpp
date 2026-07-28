#include "nova/Frontend/AST.h"
#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Parser.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using namespace nova;

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

std::unique_ptr<Program> parse(const std::string& filename,
                               const std::string& source) {
    Lexer lexer(filename, source);
    Parser parser(lexer);
    auto program = parser.parseProgram();
    if (parser.hasErrors()) {
        for (const auto& error : parser.getErrors()) {
            std::cerr << error << '\n';
        }
        ++failures;
        return nullptr;
    }
    return program;
}

FunctionDecl* functionAt(Program& program, size_t index) {
    if (index >= program.body.size()) return nullptr;
    auto* statement = dynamic_cast<DeclStmt*>(program.body[index].get());
    return statement
        ? dynamic_cast<FunctionDecl*>(statement->declaration.get())
        : nullptr;
}

void testContextualObjectMembers() {
    auto program = parse("object-members.js", R"(
        const set = {};
        const handler = {
            get(current, key) { return current[key]; },
            set(current, key, value) { current[key] = value; return true; },
            async from() { return 1; },
            *of() { yield 1; },
            [Symbol.iterator]() { return this; },
            get value() { return 1; },
            set value(next) {}
        };
        set.value = handler.value;
    )");
    if (!program) return;

    expect(program->body.size() == 3,
           "contextual identifiers must remain valid statements");
    auto* variables =
        dynamic_cast<VarDeclStmt*>(program->body[1].get());
    auto* object = variables && !variables->declarations.empty()
        ? dynamic_cast<ObjectExpr*>(
              variables->declarations[0].init.get())
        : nullptr;
    expect(object != nullptr, "handler initializer must be an object AST");
    if (!object) return;
    expect(object->properties.size() == 7,
           "all object member productions must be preserved");
    expect(object->properties[4].isComputed,
           "computed method key must be preserved");
    expect(object->properties[5].kind ==
               ObjectExpr::Property::Kind::Get,
           "getter kind must be distinguished");
    expect(object->properties[6].kind ==
               ObjectExpr::Property::Kind::Set,
           "setter kind must be distinguished");
    auto* asyncMethod =
        dynamic_cast<FunctionExpr*>(object->properties[2].value.get());
    auto* generator =
        dynamic_cast<FunctionExpr*>(object->properties[3].value.get());
    expect(asyncMethod && asyncMethod->isAsync,
           "async object method modifier must be preserved");
    expect(generator && generator->isGenerator,
           "generator object method modifier must be preserved");
}

void testDeclarationsAndOverloads() {
    auto program = parse("declarations.ts", R"(
        declare namespace Runtime {
            interface Options { readonly debug?: boolean; }
            function start(options?: Options): void;
        }
        declare const VERSION: string;
        declare class Native<T extends object = object> {
            readonly value?: T;
            close(): void;
        }
        abstract class Entity {
            abstract serialize(): string;
        }
        class User extends Entity {
            override serialize(): string { return "user"; }
        }
        function combine(left: string): string;
        function combine(left: number): number;
        function combine(left: string | number): string | number {
            return left;
        }
    )");
    if (!program) return;

    auto* namespaceStatement =
        dynamic_cast<DeclStmt*>(program->body[0].get());
    auto* namespaceDecl = namespaceStatement
        ? dynamic_cast<NamespaceDecl*>(
              namespaceStatement->declaration.get())
        : nullptr;
    expect(namespaceDecl && namespaceDecl->isDeclare,
           "declare namespace must be represented as an ambient AST node");

    FunctionDecl* firstOverload = functionAt(*program, 5);
    FunctionDecl* secondOverload = functionAt(*program, 6);
    FunctionDecl* implementation = functionAt(*program, 7);
    expect(firstOverload && firstOverload->isOverload,
           "bodyless function signature must be marked as overload");
    expect(secondOverload && secondOverload->isOverload,
           "each overload signature must be retained");
    expect(implementation && implementation->overloads.size() == 2,
           "implementation must link to all overload signatures");
    expect(firstOverload &&
               firstOverload->implementation == implementation,
           "overload must link back to implementation");
}

void testAdvancedTypes() {
    auto program = parse("advanced-types.ts", R"(
        type Element<T> = T extends readonly (infer Item)[] ? Item : never;
        type Nested = Promise<Array<string>>;
        type Getters<T> = {
            readonly [K in keyof T as `get${K}`]-?: () => T[K];
        };
        type Query = ReturnType<typeof console.log>;
        function isText(value: unknown): value is string {
            return typeof value === "string";
        }
        function assertText(value: unknown): asserts value is string {}
    )");
    if (!program) return;
    expect(program->body.size() == 6,
           "advanced type productions must all build AST nodes");

    FunctionDecl* predicate = functionAt(*program, 4);
    FunctionDecl* assertion = functionAt(*program, 5);
    expect(predicate && predicate->returnType &&
               predicate->returnType->kind ==
                   Type::Kind::TypePredicate,
           "type predicate return type must be preserved");
    expect(assertion && assertion->returnType &&
               assertion->returnType->kind ==
                   Type::Kind::TypePredicate &&
               assertion->returnType->isAssertion,
           "assertion signature must be preserved");
}

void testTSXMode() {
    auto program = parse("component.tsx", R"(
        declare namespace JSX {
            interface IntrinsicElements {
                div: { id?: string };
                "svg:path": { "data-id"?: string };
            }
        }
        const props = { id: "root" };
        const view = <>
            <div {...props}>{props.id}</div>
            <svg:path data-id="icon" />
        </>;
    )");
    if (!program) return;
    auto* variables =
        dynamic_cast<VarDeclStmt*>(program->body[2].get());
    auto* fragment = variables && !variables->declarations.empty()
        ? dynamic_cast<JSXFragment*>(
              variables->declarations[0].init.get())
        : nullptr;
    expect(fragment != nullptr,
           ".tsx extension must select JSX lexical/parser mode");
    expect(fragment && fragment->children.size() == 2,
           "fragment must preserve nested JSX children");
}

void testPhase1ConformanceSourcesBuildASTs() {
    const char* files[] = {
        "js_spec_collections.js",
        "js_spec_iterators_generators.js",
        "js_spec_proxy_reflect.js",
        "js_spec_intl.js",
        "ts_spec_structural_generics.ts",
        "ts_spec_classes_overloads.ts",
        "ts_spec_declarations.ts",
        "ts_spec_negative.ts",
        "ts_spec_type_operators.ts",
        "ts_spec_narrowing.ts",
        "ts_spec_jsx.tsx"
    };
    for (const char* file : files) {
        const std::string path =
            std::string(NOVA_SOURCE_DIR) +
            "/tests/conformance/" + file;
        std::ifstream input(path);
        std::stringstream buffer;
        buffer << input.rdbuf();
        expect(input.good() || input.eof(),
               "must read Phase 1 conformance source: " +
                   std::string(file));
        auto program = parse(path, buffer.str());
        expect(program && !program->body.empty(),
               "valid Phase 1 target must build an AST: " +
                   std::string(file));
    }
}

} // namespace

int main() {
    testContextualObjectMembers();
    testDeclarationsAndOverloads();
    testAdvancedTypes();
    testTSXMode();
    testPhase1ConformanceSourcesBuildASTs();
    if (failures == 0) {
        std::cout << "PASS: Phase 1 parser regression suite\n";
    }
    return failures == 0 ? 0 : 1;
}
