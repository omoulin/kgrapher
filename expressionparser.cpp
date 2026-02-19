#include "expressionparser.h"
#include <QtGlobal>
#include <cmath>

namespace {
double safeLog(double x) {
    if (x <= 0) return qQNaN();
    return std::log(x);
}
double safeSqrt(double x) {
    if (x < 0) return qQNaN();
    return std::sqrt(x);
}
} // namespace

ExpressionParser::Node::~Node()
{
    delete left;
    delete right;
}

void ExpressionParser::skipSpaces()
{
    while (m_pos < m_input.size() && m_input[m_pos].isSpace())
        ++m_pos;
}

bool ExpressionParser::parse(const QString &expr)
{
    m_parsed = false;
    delete m_root;
    m_root = nullptr;
    m_error.clear();
    QString normalized = expr.trimmed();
    normalized.replace(QStringLiteral("\\sin"), QStringLiteral("sin"));
    normalized.replace(QStringLiteral("\\cos"), QStringLiteral("cos"));
    normalized.replace(QStringLiteral("\\tan"), QStringLiteral("tan"));
    normalized.replace(QStringLiteral("\\sqrt"), QStringLiteral("sqrt"));
    normalized.replace(QStringLiteral("\\exp"), QStringLiteral("exp"));
    normalized.replace(QStringLiteral("\\log"), QStringLiteral("log"));
    normalized.replace(QStringLiteral("\\ln"), QStringLiteral("log"));
    m_input = normalized;
    m_pos = 0;

    Node *root = parseExpression();
    skipSpaces();
    if (m_pos != m_input.size() && m_error.isEmpty())
        m_error = QStringLiteral("Unexpected character at end");
    if (!m_error.isEmpty()) {
        delete root;
        return false;
    }
    m_root = root;
    m_parsed = true;
    return true;
}

double ExpressionParser::eval(double x) const
{
    return eval(x, 0);
}

double ExpressionParser::eval(double x, double y) const
{
    if (!m_root) return qQNaN();
    return evalNode(m_root, x, y);
}

double ExpressionParser::evalNode(const Node *n, double x, double y) const
{
    if (!n) return qQNaN();
    switch (n->type) {
    case Node::Number: return n->value;
    case Node::Variable: return x;
    case Node::VariableY: return y;
    case Node::Add: return evalNode(n->left, x, y) + evalNode(n->right, x, y);
    case Node::Sub: return evalNode(n->left, x, y) - evalNode(n->right, x, y);
    case Node::Mul: return evalNode(n->left, x, y) * evalNode(n->right, x, y);
    case Node::Div: {
        double d = evalNode(n->right, x, y);
        return d == 0 ? qQNaN() : evalNode(n->left, x, y) / d;
    }
    case Node::Pow: return std::pow(evalNode(n->left, x, y), evalNode(n->right, x, y));
    case Node::Negate: return -evalNode(n->left, x, y);
    case Node::Sin: return std::sin(evalNode(n->left, x, y));
    case Node::Cos: return std::cos(evalNode(n->left, x, y));
    case Node::Tan: return std::tan(evalNode(n->left, x, y));
    case Node::Sqrt: return safeSqrt(evalNode(n->left, x, y));
    case Node::Exp: return std::exp(evalNode(n->left, x, y));
    case Node::Log: return safeLog(evalNode(n->left, x, y));
    }
    return qQNaN();
}

ExpressionParser::Node *ExpressionParser::parseExpression()
{
    Node *left = parseTerm();
    if (!left && !m_error.isEmpty()) return nullptr;
    skipSpaces();
    while (m_pos < m_input.size()) {
        QChar c = m_input[m_pos];
        if (c == QLatin1Char('+')) {
            ++m_pos;
            skipSpaces();
            Node *right = parseTerm();
            if (!right) { delete left; return nullptr; }
            Node *n = new Node;
            n->type = Node::Add;
            n->left = left;
            n->right = right;
            left = n;
        } else if (c == QLatin1Char('-')) {
            ++m_pos;
            skipSpaces();
            Node *right = parseTerm();
            if (!right) { delete left; return nullptr; }
            Node *n = new Node;
            n->type = Node::Sub;
            n->left = left;
            n->right = right;
            left = n;
        } else
            break;
        skipSpaces();
    }
    return left;
}

ExpressionParser::Node *ExpressionParser::parseTerm()
{
    Node *left = parseFactor();
    if (!left && !m_error.isEmpty()) return nullptr;
    skipSpaces();
    while (m_pos < m_input.size()) {
        QChar c = m_input[m_pos];
        if (c == QLatin1Char('*')) {
            ++m_pos;
            skipSpaces();
            Node *right = parseFactor();
            if (!right) { delete left; return nullptr; }
            Node *n = new Node;
            n->type = Node::Mul;
            n->left = left;
            n->right = right;
            left = n;
        } else if (c == QLatin1Char('/')) {
            ++m_pos;
            skipSpaces();
            Node *right = parseFactor();
            if (!right) { delete left; return nullptr; }
            Node *n = new Node;
            n->type = Node::Div;
            n->left = left;
            n->right = right;
            left = n;
        } else
            break;
        skipSpaces();
    }
    return left;
}

ExpressionParser::Node *ExpressionParser::parseFactor()
{
    return parsePower();
}

ExpressionParser::Node *ExpressionParser::parsePower()
{
    Node *base = parseUnary();
    if (!base && !m_error.isEmpty()) return nullptr;
    skipSpaces();
    if (m_pos < m_input.size() && m_input[m_pos] == QLatin1Char('^')) {
        ++m_pos;
        skipSpaces();
        Node *exp = parsePower();
        if (!exp) { delete base; return nullptr; }
        Node *n = new Node;
        n->type = Node::Pow;
        n->left = base;
        n->right = exp;
        return n;
    }
    // Implicit multiplication: 2x, xy, x(1+2), etc.
    if (m_pos < m_input.size()) {
        QChar c = m_input[m_pos];
        bool implicit = c == QLatin1Char('x') || c == QLatin1Char('X')
            || c == QLatin1Char('y') || c == QLatin1Char('Y') || c == QLatin1Char('(')
            || c.isDigit() || c == QLatin1Char('.');
        if (!implicit && (c == 's' || c == 'c' || c == 't' || c == 'e' || c == 'l')) {
            QString rest = m_input.mid(m_pos);
            implicit = rest.startsWith(QLatin1String("sin"), Qt::CaseInsensitive)
                || rest.startsWith(QLatin1String("cos"), Qt::CaseInsensitive)
                || rest.startsWith(QLatin1String("tan"), Qt::CaseInsensitive)
                || rest.startsWith(QLatin1String("sqrt"), Qt::CaseInsensitive)
                || rest.startsWith(QLatin1String("exp"), Qt::CaseInsensitive)
                || rest.startsWith(QLatin1String("log"), Qt::CaseInsensitive);
        }
        if (implicit) {
            Node *right = parseUnary();
            if (right) {
                Node *n = new Node;
                n->type = Node::Mul;
                n->left = base;
                n->right = right;
                return n;
            }
        }
    }
    return base;
}

ExpressionParser::Node *ExpressionParser::parseUnary()
{
    skipSpaces();
    if (m_pos < m_input.size() && m_input[m_pos] == QLatin1Char('-')) {
        ++m_pos;
        Node *child = parseUnary();
        if (!child) return nullptr;
        Node *n = new Node;
        n->type = Node::Negate;
        n->left = child;
        return n;
    }
    return parsePrimary();
}

ExpressionParser::Node *ExpressionParser::parseFunction(const QString &name)
{
    skipSpaces();
    if (m_pos >= m_input.size() || m_input[m_pos] != QLatin1Char('(')) {
        m_error = QStringLiteral("Expected '(' after '%1'").arg(name);
        return nullptr;
    }
    ++m_pos;
    Node *arg = parseExpression();
    if (!arg) return nullptr;
    skipSpaces();
    if (m_pos >= m_input.size() || m_input[m_pos] != QLatin1Char(')')) {
        delete arg;
        m_error = QStringLiteral("Expected ')'");
        return nullptr;
    }
    ++m_pos;

    Node *n = new Node;
    n->left = arg;
    if (name == QLatin1String("sin")) n->type = Node::Sin;
    else if (name == QLatin1String("cos")) n->type = Node::Cos;
    else if (name == QLatin1String("tan")) n->type = Node::Tan;
    else if (name == QLatin1String("sqrt")) n->type = Node::Sqrt;
    else if (name == QLatin1String("exp")) n->type = Node::Exp;
    else if (name == QLatin1String("log")) n->type = Node::Log;
    else { m_error = QStringLiteral("Unknown function: %1").arg(name); delete n; return nullptr; }
    return n;
}

ExpressionParser::Node *ExpressionParser::parsePrimary()
{
    skipSpaces();
    if (m_pos >= m_input.size()) {
        m_error = QStringLiteral("Unexpected end of expression");
        return nullptr;
    }

    if (m_input[m_pos] == QLatin1Char('x') || m_input[m_pos] == QLatin1Char('X')) {
        Node *n = new Node;
        n->type = Node::Variable;
        ++m_pos;
        return n;
    }

    if (m_input[m_pos] == QLatin1Char('y') || m_input[m_pos] == QLatin1Char('Y')) {
        Node *n = new Node;
        n->type = Node::VariableY;
        ++m_pos;
        return n;
    }

    if (m_input[m_pos] == QLatin1Char('(')) {
        ++m_pos;
        Node *inner = parseExpression();
        if (!inner) return nullptr;
        skipSpaces();
        if (m_pos >= m_input.size() || m_input[m_pos] != QLatin1Char(')')) {
            delete inner;
            m_error = QStringLiteral("Expected ')'");
            return nullptr;
        }
        ++m_pos;
        return inner;
    }

    if (m_input[m_pos].isDigit() || (m_input[m_pos] == QLatin1Char('.') && m_pos + 1 < m_input.size() && m_input[m_pos + 1].isDigit())) {
        int start = m_pos;
        if (m_input[m_pos] == QLatin1Char('.')) ++m_pos;
        while (m_pos < m_input.size() && (m_input[m_pos].isDigit() || m_input[m_pos] == QLatin1Char('.')))
            ++m_pos;
        bool ok = false;
        double v = m_input.mid(start, m_pos - start).toDouble(&ok);
        if (!ok) { m_error = QStringLiteral("Invalid number"); return nullptr; }
        Node *n = new Node;
        n->type = Node::Number;
        n->value = v;
        return n;
    }

    static const char *funcs[] = { "sin", "cos", "tan", "sqrt", "exp", "log" };
    for (const char *f : funcs) {
        QString fn = QString::fromLatin1(f);
        if (m_pos + fn.size() <= m_input.size() && m_input.mid(m_pos, fn.size()).compare(fn, Qt::CaseInsensitive) == 0) {
            m_pos += fn.size();
            return parseFunction(fn);
        }
    }

    m_error = QStringLiteral("Unexpected character '%1'").arg(m_input[m_pos]);
    return nullptr;
}
