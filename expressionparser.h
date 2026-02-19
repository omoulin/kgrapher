#ifndef EXPRESSIONPARSER_H
#define EXPRESSIONPARSER_H

#include <QString>
#include <QVector>

class ExpressionParser
{
public:
    ExpressionParser() = default;

    bool parse(const QString &expr);
    double eval(double x) const;
    double eval(double x, double y) const;
    QString errorString() const { return m_error; }
    bool isValid() const { return m_parsed; }

private:
    struct Node {
        enum Type { Number, Variable, VariableY, Add, Sub, Mul, Div, Pow, Negate,
                    Sin, Cos, Tan, Sqrt, Exp, Log } type;
        double value = 0;
        Node *left = nullptr;
        Node *right = nullptr;
        ~Node();
    };

    void skipSpaces();
    Node *parseExpression();
    Node *parseTerm();
    Node *parseFactor();
    Node *parsePower();
    Node *parseUnary();
    Node *parsePrimary();
    Node *parseFunction(const QString &name);
    double evalNode(const Node *n, double x, double y) const;

    QString m_input;
    int m_pos = 0;
    QString m_error;
    bool m_parsed = false;
    Node *m_root = nullptr;
};

#endif // EXPRESSIONPARSER_H
