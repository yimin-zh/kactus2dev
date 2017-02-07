//-----------------------------------------------------------------------------
// File: VerilogSyntax.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Janne Virtanen
// Date: 20.10.2016
//
// Description:
// Common elements of the Verilog language.
//-----------------------------------------------------------------------------

#include "VerilogSyntax.h"

#include <QTextStream>
#include <QFile>

//-----------------------------------------------------------------------------
// Function: VerilogSyntax::cullStrayComments()
//-----------------------------------------------------------------------------
QString VerilogSyntax::cullStrayComments(QString const& input)
{
    // Make a copy, remove all multiline comments as well as lines that are purely comments.
    QString inspectWithoutComments = input;
    inspectWithoutComments.remove(VerilogSyntax::MULTILINE_COMMENT);
    inspectWithoutComments.remove(VerilogSyntax::COMMENTLINE);

    return inspectWithoutComments;
}

//-----------------------------------------------------------------------------
// Function: VerilogSyntax::findModuleHeader()
//-----------------------------------------------------------------------------
QPair<int,int> VerilogSyntax::findModuleDeclaration(QString const& input)
{
    // The return value.
    QPair<int,int> retval;

    // Find the beginning of the module header.
    int moduleDeclarationBeginIndex = input.indexOf(VerilogSyntax::MODULE_KEY_WORD);

    // Its shall be the first return value.
    retval.first = moduleDeclarationBeginIndex;

    // Make a copy of the input.
    QString inspect = input;

    // The ending position of the module header.
    int moduleDeclarationEndIndex = -1;
    
    // We decide it is easier to read in stream format.
    QTextStream sourceStream(&inspect);
    // The last stream position.
    int streamPos = -1;

    // The alternative syntaxes for comment starts.
    QRegularExpression comment("(////)|(//*)");

    while (!sourceStream.atEnd())
    {
        // Remember the position before this line.
        streamPos = sourceStream.pos();

        // Read the next line.
        const QString currentLine = sourceStream.readLine();

        // Find position of any comment start in the line, as well as declaration ending.
        int commentPosition = currentLine.indexOf(comment);
        int possibleEnding = currentLine.indexOf(");");

        // If the declaration ending is actually before the comment, it is now valid.
        if (commentPosition != -1 && commentPosition < possibleEnding)
        {
            continue;
        }

        // If the ending is detected, we are done looping.
        if (possibleEnding != -1)
        {
            moduleDeclarationEndIndex = possibleEnding;
            break;
        }
    }

    // No ending for module declaration detected: Set return value as -1.
    if (moduleDeclarationEndIndex == -1)
    {
        retval.second = -1;
        return retval;
    }

    // The position for the modue declaration ending = position on the line + stream position before the line.
    moduleDeclarationEndIndex = moduleDeclarationEndIndex + streamPos;
    // Set it as a return value.
    retval.second = moduleDeclarationEndIndex -  moduleDeclarationBeginIndex;

    return retval;
}

//-----------------------------------------------------------------------------
// Function: VerilogSyntax::readImplementation()
//-----------------------------------------------------------------------------
bool VerilogSyntax::readImplementation(QString const& outputPath, QString& implementation,
    QString& postModule, QString& error)
{
    // Check if the output file already exists.
    QFile outputFile(outputPath); 

    // If it does not, there is nothing do here.
    if (!outputFile.exists())
    {
        return true;
    }

    // Must be able to open it for reading.
    if (!outputFile.open(QIODevice::ReadOnly))
    {
        error = QObject::tr("The output file exists, but could not open it for reading. The path: %1").arg(outputPath);
        return false;
    }

    // Read the content.
    QTextStream inputStream(&outputFile);
    QString fileContent = inputStream.readAll();

    return selectImplementation(fileContent, implementation, postModule, error);
}

//-----------------------------------------------------------------------------
// Function: VerilogSyntax::selectImplementation()
//-----------------------------------------------------------------------------
bool VerilogSyntax::selectImplementation(QString const& fileContent, QString& implementation,
    QString& postModule, QString& error)
{
    int implementationStart;
    int implementationEnd;

    if (!findImplementation(fileContent, implementationStart, implementationEnd, error))
    {
        return false;
    }

    // Rip the implementation once detected.
    int implementationLength = implementationEnd - implementationStart;
    implementation = fileContent.mid(implementationStart,implementationLength);

    // Then take all the text that comes after the module, just in case.
    int postStart = implementationEnd + 9;
    postModule = fileContent.mid(postStart);

    // Also trim away extra white space.
    postModule = postModule.trimmed();

    // The destructor shall close the file. All done here.
    return true;
}

//-----------------------------------------------------------------------------
// Function: VerilogSyntax::findImplementation()
//-----------------------------------------------------------------------------
bool VerilogSyntax::findImplementation(QString const& fileContent, int& implementationStart,
    int& implementationEnd, QString& error)
{
    // We do not support multiple modules in the same file.
    if (fileContent.count(VerilogSyntax::MODULE_KEY_WORD) > 1)
    {
        error = QObject::tr("There was more than one module headers in the file!");
        return false;
    }

    // Find the module header.
    QPair<int,int> headerPosition = VerilogSyntax::findModuleDeclaration(fileContent);
    int moduleDeclarationBeginIndex = headerPosition.first;
    int moduleDeclarationLength = headerPosition.second;

    // Must have it to proceed.
    if (moduleDeclarationBeginIndex == -1)
    {
        error = QObject::tr("Could not find module header start from the file.");
        return false;
    }

    // Must have it to proceed.
    if (moduleDeclarationLength == -1)
    {
        error = QObject::tr("Could not find module header end from the file!");
        return false;
    }

    // The end of the override tag line is the beginning of the implementation.
    implementationStart = fileContent.indexOf(VerilogSyntax::TAG_OVERRIDE);

    if (implementationStart == -1)
    {
        // If does not exist, the end of the header is the beginning of the implementation.
        implementationStart = moduleDeclarationBeginIndex + moduleDeclarationLength + 3;
    }
    else
    {
        // Else we should seek the position where tag line ends.
        implementationStart += VerilogSyntax::TAG_OVERRIDE.size() + 1;
    }

    // The end of the module is the end of the implementation.
    implementationEnd = fileContent.indexOf(VerilogSyntax::MODULE_END);

    // The module must end some where.
    if (implementationEnd == -1)
    {
        error = QObject::tr("Could not find module end from the file!");
        return false;
    }

    // The destructor shall close the file. All done here.
    return true;
}