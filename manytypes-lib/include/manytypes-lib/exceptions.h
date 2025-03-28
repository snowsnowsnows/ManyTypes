#pragma once

#include <stdexcept>
#include <string>

namespace mt
{

class Exception : public std::runtime_error
{
public:
    explicit Exception(const std::string& message)
        : std::runtime_error(message) {}
};

class DatabaseException : public Exception
{
public:
    explicit DatabaseException(const std::string& message)
        : Exception("DatabaseException: " + message) {}
};

class InvalidSemanticParentException : public DatabaseException
{
public:
    explicit InvalidSemanticParentException(const std::string& message)
        : DatabaseException("InvalidSemanticParentException: " + message) {}
};

class TypeNotFoundException : public DatabaseException
{
public:
    explicit TypeNotFoundException(const std::string& message)
        : DatabaseException("TypeNotFoundException: " + message) {}
};

class TypeAlreadyExistsException : public DatabaseException
{
public:
    explicit TypeAlreadyExistsException(const std::string& message)
        : DatabaseException("TypeAlreadyExistsException: " + message) {}
};

class FormatterException : public Exception
{
public:
    explicit FormatterException(const std::string& message)
        : Exception("FormatterException: " + message) {}
};

class CircularDependencyException : public FormatterException
{
public:
    explicit CircularDependencyException(const std::string& message)
        : FormatterException("CircularDependencyException: " + message) {}
};

class ClangFormatterException : public FormatterException
{
public:
    explicit ClangFormatterException(const std::string& message)
        : FormatterException("ClangFormatterException: " + message) {}
};

class UnknownTypeException : public ClangFormatterException
{
public:
    explicit UnknownTypeException(const std::string& message)
        : ClangFormatterException("UnknownTypeException: " + message) {}
};

class InvalidTypeException : public ClangFormatterException
{
public:
    explicit InvalidTypeException(const std::string& message)
        : ClangFormatterException("InvalidTypeException: " + message) {}
};

class X64DbgFormatterException : public FormatterException
{
public:
    explicit X64DbgFormatterException(const std::string& message)
        : FormatterException("X64DbgFormatterException: " + message) {}
};

class InvalidPointerSizeException : public X64DbgFormatterException
{
public:
    explicit InvalidPointerSizeException(const std::string& message)
        : X64DbgFormatterException("InvalidPointerSizeException: " + message) {}
};

class X64DbgUnknownTypeException : public X64DbgFormatterException
{
public:
    explicit X64DbgUnknownTypeException(const std::string& message)
        : X64DbgFormatterException("X64DbgUnknownTypeException: " + message) {}
};

class ParserException : public Exception
{
public:
    explicit ParserException(const std::string& message)
        : Exception("ParserException: " + message) {}
};

class TypeNotDefinedException : public ParserException
{
public:
    explicit TypeNotDefinedException(const std::string& message)
        : ParserException("TypeNotDefinedException: " + message) {}
};

class InvalidStructureException : public ParserException
{
public:
    explicit InvalidStructureException(const std::string& message)
        : ParserException("InvalidStructureException: " + message) {}
};

class InvalidParentDeclarationException : public ParserException
{
public:
    explicit InvalidParentDeclarationException(const std::string& message)
        : ParserException("InvalidParentDeclarationException: " + message) {}
};

class InvalidFieldException : public ParserException
{
public:
    explicit InvalidFieldException(const std::string& message)
        : ParserException("InvalidFieldException: " + message) {}
};

class UnsupportedScopeException : public ParserException
{
public:
    explicit UnsupportedScopeException(const std::string& message)
        : ParserException("UnsupportedScopeException: " + message) {}
};

} // namespace mt
