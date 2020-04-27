//-----------------------------------------------------------------------------
// File: ParameterizableInterface.h
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Mikko Teuho
// Date: 13.03.2020
//
// Description:
// Interface for parameterizable items.
//-----------------------------------------------------------------------------

#ifndef PARAMETERIZABLEINTERFACE_H
#define PARAMETERIZABLEINTERFACE_H

#include <editors/ComponentEditor/common/ExpressionFormatter.h>
#include <editors/ComponentEditor/common/ParameterFinder.h>

class ExpressionFormatter;
class ValueFormatter;

//-----------------------------------------------------------------------------
//! Interface for parameterizable items.
//-----------------------------------------------------------------------------
class ParameterizableInterface
{
public:

    /*!
     *  The constructor.
     */
    ParameterizableInterface();

	/*!
     *  The destructor.
     */
    virtual ~ParameterizableInterface() = default;

    /*!
     *  Set expression parser.
     *
     *      @param [in] parser  Parser for expressions.
     */
    void setExpressionParser(QSharedPointer<ExpressionParser> parser);

    /*!
     *  Set expression formatter.
     *
     *      @param [in] formatter   Formatter for expressions.
     */
    void setExpressionFormatter(QSharedPointer<ExpressionFormatter> formatter);

    /*!
     *  Calculate all the references to the selected ID in the selected item.
     *
     *      @param [in] itemName    Name of the selected item.
     *      @param [in] valueID     The selected ID.
     *
     *      @return Number of references to the selected ID in the selected item.
     */
    virtual int getAllReferencesToIdInItem(const std::string& itemName, std::string const&  valueID) const = 0;

    //! No copying. No assignment.
    ParameterizableInterface(const ParameterizableInterface& other) = delete;
    ParameterizableInterface& operator=(const ParameterizableInterface& other) = delete;

protected:

    /*!
     *  Get the formatted value for the given expression.
     *
     *      @param [in] expression  The selected expression.
     *
     *      @return The formatted expression.
     */
    QString formattedValueFor(QString const& expression) const;

    /*!
     *  Parse the selected expression to decimal.
     *
     *      @param [in] expression  The selected expression.
     *
     *      @return The value of the expression in decimal form.
     */
    QString parseExpressionToDecimal(QString const& expression) const;

    /*!
     *  Parse the selected expression to the selected base number.
     *
     *      @param [in] expression  The selected expression.
     *      @param [in] baseNumber  The selected base number.
     *
     *      @return The value of the expression in the base number form.
     */
    QString parseExpressionToBaseNumber(QString const& expression, int const& baseNumber) const;

private:

    //-----------------------------------------------------------------------------
    // Data.
    //-----------------------------------------------------------------------------

    //! Expression parser for solving expressions.
    QSharedPointer<ExpressionParser> expressionParser_;

    //! Expression formatter, formats the referencing expressions to show parameter names.
    QSharedPointer<ExpressionFormatter> formatter_;

    //! Formatter for values.
    QSharedPointer<ValueFormatter> valueFormatter_;
};

#endif // PARAMETERIZABLEINTERFACE_H