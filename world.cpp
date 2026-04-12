#include <random>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <complex>
#include <libndls.h>

#include "blockrenderer.h"
#include "world.h"
#include "chunk.h"
#include "inventorytask.h"
#include "settingstask.h"

World world;

namespace {
enum class TokenType {
    Number,
    VarX,
    VarY,
    Add,
    Sub,
    Mul,
    Div,
    Pow,
    Neg,
    FuncSin,
    FuncCos,
    FuncTan,
    FuncAbs,
    FuncSqrt,
    LParen,
    RParen
};

struct Token {
    TokenType type;
    double number;
};

static bool isValueToken(const TokenType t)
{
    return t == TokenType::Number || t == TokenType::VarX || t == TokenType::VarY || t == TokenType::RParen;
}

static bool startsValueToken(const TokenType t)
{
    return t == TokenType::Number || t == TokenType::VarX || t == TokenType::VarY || t == TokenType::LParen
        || t == TokenType::FuncSin || t == TokenType::FuncCos || t == TokenType::FuncTan
        || t == TokenType::FuncAbs || t == TokenType::FuncSqrt;
}

static bool isFunctionToken(const TokenType t)
{
    return t == TokenType::FuncSin || t == TokenType::FuncCos || t == TokenType::FuncTan
        || t == TokenType::FuncAbs || t == TokenType::FuncSqrt || t == TokenType::Neg;
}

static int precedence(const TokenType t)
{
    switch(t)
    {
    case TokenType::Add:
    case TokenType::Sub:
        return 1;
    case TokenType::Mul:
    case TokenType::Div:
        return 2;
    case TokenType::Pow:
        return 3;
    case TokenType::Neg:
        return 4;
    default:
        return -1;
    }
}

static bool rightAssociative(const TokenType t)
{
    return t == TokenType::Pow || t == TokenType::Neg;
}

static std::string normalizeGraphExpression(const char *expression)
{
    std::string s = expression ? expression : "";
    std::string compact;
    compact.reserve(s.size());

    for(char ch : s)
    {
        if(!std::isspace(static_cast<unsigned char>(ch)))
            compact.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    const size_t eq = compact.find('=');
    if(eq != std::string::npos)
        compact = compact.substr(eq + 1);

    if(compact.empty())
        compact = "sin(x)";

    auto starts_with = [&compact](const char *prefix) {
        const size_t n = std::strlen(prefix);
        return compact.size() > n && compact.compare(0, n, prefix) == 0;
    };

    if(starts_with("sin") && compact[3] != '(')
        compact = "sin(" + compact.substr(3) + ")";
    else if(starts_with("cos") && compact[3] != '(')
        compact = "cos(" + compact.substr(3) + ")";
    else if(starts_with("tan") && compact[3] != '(')
        compact = "tan(" + compact.substr(3) + ")";

    return compact;
}

static bool tokenizeGraphExpression(const std::string &expr, std::vector<Token> &tokens)
{
    tokens.clear();

    for(size_t i = 0; i < expr.size();)
    {
        const char ch = expr[i];

        if((ch >= '0' && ch <= '9') || ch == '.')
        {
            size_t j = i;
            bool dot_seen = false;
            while(j < expr.size())
            {
                const char cj = expr[j];
                if(cj == '.')
                {
                    if(dot_seen)
                        return false;
                    dot_seen = true;
                    ++j;
                    continue;
                }
                if(cj < '0' || cj > '9')
                    break;
                ++j;
            }
            tokens.push_back({TokenType::Number, std::atof(expr.substr(i, j - i).c_str())});
            i = j;
            continue;
        }

        if((ch >= 'a' && ch <= 'z'))
        {
            size_t j = i;
            while(j < expr.size() && (expr[j] >= 'a' && expr[j] <= 'z'))
                ++j;

            const std::string ident = expr.substr(i, j - i);
            if(ident == "x")
                tokens.push_back({TokenType::VarX, 0.0});
            else if(ident == "y" || ident == "z")
                tokens.push_back({TokenType::VarY, 0.0});
            else if(ident == "pi")
                tokens.push_back({TokenType::Number, 3.14159265358979323846});
            else if(ident == "e")
                tokens.push_back({TokenType::Number, 2.71828182845904523536});
            else if(ident == "sin")
                tokens.push_back({TokenType::FuncSin, 0.0});
            else if(ident == "cos")
                tokens.push_back({TokenType::FuncCos, 0.0});
            else if(ident == "tan")
                tokens.push_back({TokenType::FuncTan, 0.0});
            else if(ident == "abs")
                tokens.push_back({TokenType::FuncAbs, 0.0});
            else if(ident == "sqrt")
                tokens.push_back({TokenType::FuncSqrt, 0.0});
            else
            {
                // Allow compact variable spelling like "xy" or "xzy".
                bool only_vars = true;
                for(char c : ident)
                {
                    if(c != 'x' && c != 'y' && c != 'z')
                    {
                        only_vars = false;
                        break;
                    }
                }
                if(!only_vars)
                    return false;

                for(char c : ident)
                {
                    if(c == 'x')
                        tokens.push_back({TokenType::VarX, 0.0});
                    else
                        tokens.push_back({TokenType::VarY, 0.0});
                }
            }

            i = j;
            continue;
        }

        if(ch == '+')
            tokens.push_back({TokenType::Add, 0.0});
        else if(ch == '-')
            tokens.push_back({TokenType::Sub, 0.0});
        else if(ch == '*')
            tokens.push_back({TokenType::Mul, 0.0});
        else if(ch == '/')
            tokens.push_back({TokenType::Div, 0.0});
        else if(ch == '^')
            tokens.push_back({TokenType::Pow, 0.0});
        else if(ch == '(')
            tokens.push_back({TokenType::LParen, 0.0});
        else if(ch == ')')
            tokens.push_back({TokenType::RParen, 0.0});
        else
            return false;

        ++i;
    }

    // Insert implicit multiplication (e.g. 2x, x(y), xz, cos(x)y).
    std::vector<Token> expanded;
    expanded.reserve(tokens.size() * 2);
    for(size_t i = 0; i < tokens.size(); ++i)
    {
        if(!expanded.empty())
        {
            const TokenType prev = expanded.back().type;
            const TokenType curr = tokens[i].type;
            if(isValueToken(prev) && startsValueToken(curr))
                expanded.push_back({TokenType::Mul, 0.0});
        }
        expanded.push_back(tokens[i]);
    }
    tokens.swap(expanded);

    // Mark unary minus as NEG.
    for(size_t i = 0; i < tokens.size(); ++i)
    {
        if(tokens[i].type != TokenType::Sub)
            continue;

        if(i == 0)
        {
            tokens[i].type = TokenType::Neg;
            continue;
        }

        const TokenType prev = tokens[i - 1].type;
        if(!(isValueToken(prev) || prev == TokenType::RParen))
            tokens[i].type = TokenType::Neg;
    }

    return !tokens.empty();
}

static bool toRpn(const std::vector<Token> &tokens, std::vector<Token> &output)
{
    output.clear();
    std::vector<Token> stack;

    for(const Token &t : tokens)
    {
        switch(t.type)
        {
        case TokenType::Number:
        case TokenType::VarX:
        case TokenType::VarY:
            output.push_back(t);
            break;

        case TokenType::FuncSin:
        case TokenType::FuncCos:
        case TokenType::FuncTan:
        case TokenType::FuncAbs:
        case TokenType::FuncSqrt:
        case TokenType::Neg:
            stack.push_back(t);
            break;

        case TokenType::Add:
        case TokenType::Sub:
        case TokenType::Mul:
        case TokenType::Div:
        case TokenType::Pow:
            while(!stack.empty())
            {
                const TokenType top = stack.back().type;
                if(isFunctionToken(top))
                {
                    output.push_back(stack.back());
                    stack.pop_back();
                    continue;
                }
                if(top == TokenType::LParen)
                    break;

                const int p1 = precedence(t.type);
                const int p2 = precedence(top);
                if((!rightAssociative(t.type) && p1 <= p2) || (rightAssociative(t.type) && p1 < p2))
                {
                    output.push_back(stack.back());
                    stack.pop_back();
                    continue;
                }
                break;
            }
            stack.push_back(t);
            break;

        case TokenType::LParen:
            stack.push_back(t);
            break;

        case TokenType::RParen:
            while(!stack.empty() && stack.back().type != TokenType::LParen)
            {
                output.push_back(stack.back());
                stack.pop_back();
            }
            if(stack.empty() || stack.back().type != TokenType::LParen)
                return false;
            stack.pop_back();
            if(!stack.empty() && isFunctionToken(stack.back().type))
            {
                output.push_back(stack.back());
                stack.pop_back();
            }
            break;
        }
    }

    while(!stack.empty())
    {
        if(stack.back().type == TokenType::LParen || stack.back().type == TokenType::RParen)
            return false;
        output.push_back(stack.back());
        stack.pop_back();
    }

    return !output.empty();
}

static bool evaluateRpn(const std::vector<Token> &rpn, const double x, const double y, double &out)
{
    std::vector<double> stack;
    stack.reserve(rpn.size());

    for(const Token &t : rpn)
    {
        switch(t.type)
        {
        case TokenType::Number:
            stack.push_back(t.number);
            break;
        case TokenType::VarX:
            stack.push_back(x);
            break;
        case TokenType::VarY:
            stack.push_back(y);
            break;

        case TokenType::Neg:
        case TokenType::FuncSin:
        case TokenType::FuncCos:
        case TokenType::FuncTan:
        case TokenType::FuncAbs:
        case TokenType::FuncSqrt:
            if(stack.empty())
                return false;
            {
                const double v = stack.back();
                stack.pop_back();
                double r = 0.0;
                if(t.type == TokenType::Neg)
                    r = -v;
                else if(t.type == TokenType::FuncSin)
                    r = std::sin(v);
                else if(t.type == TokenType::FuncCos)
                    r = std::cos(v);
                else if(t.type == TokenType::FuncTan)
                    r = std::tan(v);
                else if(t.type == TokenType::FuncAbs)
                    r = std::fabs(v);
                else
                    r = std::sqrt(v);
                stack.push_back(r);
            }
            break;

        case TokenType::Add:
        case TokenType::Sub:
        case TokenType::Mul:
        case TokenType::Div:
        case TokenType::Pow:
            if(stack.size() < 2)
                return false;
            {
                const double rhs = stack.back();
                stack.pop_back();
                const double lhs = stack.back();
                stack.pop_back();

                double r = 0.0;
                if(t.type == TokenType::Add)
                    r = lhs + rhs;
                else if(t.type == TokenType::Sub)
                    r = lhs - rhs;
                else if(t.type == TokenType::Mul)
                    r = lhs * rhs;
                else if(t.type == TokenType::Div)
                {
                    if(std::fabs(rhs) < 1e-12)
                        return false;
                    r = lhs / rhs;
                }
                else
                    r = std::pow(lhs, rhs);

                stack.push_back(r);
            }
            break;

        default:
            return false;
        }
    }

    if(stack.size() != 1)
        return false;

    out = stack.back();
    return std::isfinite(out);
}

static BLOCK phaseToWool(double phase)
{
    static const BLOCK palette[] = {
        BLOCK_WOOL_RED,
        BLOCK_WOOL_ORANGE,
        BLOCK_WOOL_YELLOW,
        BLOCK_WOOL_GREEN,
        BLOCK_WOOL_CYAN,
        BLOCK_WOOL_LIGHT_BLUE,
        BLOCK_WOOL_DARK_BLUE,
        BLOCK_WOOL_PURPLE,
        BLOCK_WOOL_MAGENTA,
        BLOCK_WOOL_PINK,
        BLOCK_WOOL_WHITE,
        BLOCK_WOOL_GRAY,
        BLOCK_WOOL_BLACK,
        BLOCK_WOOL_BROWN,
        BLOCK_WOOL_DARK_GREEN,
    };

    double t = (phase + 3.14159265358979323846) / (2.0 * 3.14159265358979323846);
    if(t < 0.0)
        t = 0.0;
    if(t >= 1.0)
        t = std::nextafter(1.0, 0.0);
    const double palette_count = static_cast<double>(sizeof(palette) / sizeof(*palette));
    const int idx = static_cast<int>(t * palette_count);
    return palette[idx];
}

static bool evaluateComplexPreset(const std::string &expr, double x, double y, std::complex<double> &out)
{
    const std::complex<double> z(x, y);
    if(expr == "c:z^2")
        out = z * z;
    else if(expr == "c:z^3-1")
        out = z * z * z - std::complex<double>(1.0, 0.0);
    else if(expr == "c:1/z")
    {
        if(std::abs(z) < 1e-8)
            return false;
        out = 1.0 / z;
    }
    else if(expr == "c:sin(z)")
        out = std::sin(z);
    else if(expr == "c:(z^2+1)/(z^2-1)")
    {
        const std::complex<double> z2 = z * z;
        const std::complex<double> d = z2 - std::complex<double>(1.0, 0.0);
        if(std::abs(d) < 1e-8)
            return false;
        out = (z2 + std::complex<double>(1.0, 0.0)) / d;
    }
    else
        return false;

    return std::isfinite(out.real()) && std::isfinite(out.imag());
}

static bool evaluateImplicitPreset(const std::string &expr, double x, double z, std::vector<double> &ys)
{
    ys.clear();

    if(expr == "i:sphere")
    {
        // x^2 + y^2 + z^2 = r^2
        constexpr double r = 2.5;
        const double inner = r * r - x * x - z * z;
        if(inner < -0.5)  // allow slight boundary overshoot for discrete grid coverage
            return false;
        const double y = std::sqrt(std::max(0.0, inner));  // clamp to avoid NaN
        ys.push_back(y);
        if(y > 1e-10)  // reduced threshold to catch smaller values
            ys.push_back(-y);
        return true;
    }

    if(expr == "i:toroid")
    {
        // (sqrt(x^2+z^2)-R)^2 + y^2 = r^2
        constexpr double R = 2.0;
        constexpr double r = 0.8;
        constexpr double edge_tolerance = 1.2;  // larger tolerance for diagonal edges
        const double radial = std::sqrt(x * x + z * z);
        const double inner = r * r - (radial - R) * (radial - R);
        if(inner < -0.3)  // allow slight boundary overshoot for discrete grid coverage
            return false;
        const double y = std::sqrt(std::max(0.0, inner));  // clamp to avoid NaN
        if(y > 1e-10)  // reduced threshold to catch smaller values
            ys.push_back(-y);
        return true;
    }

    return false;
}
}

World::World() : perlin_noise(0)
{
    generateSeed();
    rebuildGraphPoints();
    setFieldOfView(field_of_view);
}

World::~World()
{
    for(auto&& c : all_chunks)
        delete c.second;
}

void World::generateSeed()
{
    seed = static_cast<unsigned int>(rand());
    printf("Seed: %u\n", seed);
    perlin_noise.setSeed(seed);
}

bool World::setGraphExpression(const char *expression)
{
    const std::string normalized = normalizeGraphExpression(expression);

    if(!normalized.empty() && normalized.rfind("c:", 0) == 0)
        return setComplexGraphExpression(normalized);
    if(!normalized.empty() && normalized.rfind("i:", 0) == 0)
        return setImplicitGraphExpression(normalized);

    std::vector<Token> tokens;
    std::vector<Token> rpn;
    if(!tokenizeGraphExpression(normalized, tokens) || !toRpn(tokens, rpn))
        return false;

    // Validate by evaluating at origin.
    double value = 0.0;
    if(!evaluateRpn(rpn, 0.0, 0.0, value))
        return false;

    graph_mode = GraphMode::Real;
    graph_expression = normalized;
    rebuildGraphPoints();
    graph_reveal_x = graph_range;
    graph_line_sweep_enabled = false;
    return true;
}

bool World::setGraphZoomPercent(int zoom_percent)
{
    if(zoom_percent < 20 || zoom_percent > 800)
        return false;

    graph_zoom_percent = zoom_percent;
    rebuildGraphPoints();
    return true;
}

bool World::setGraphFillDepth(int fill_depth)
{
    if(fill_depth < 1 || fill_depth > 32)
        return false;

    graph_fill_depth = fill_depth;
    return true;
}

void World::setGraphUnbounded(bool enabled)
{
    if(graph_unbounded == enabled)
        return;

    graph_unbounded = enabled;
    rebuildGraphPoints();
}

bool World::setComplexGraphExpression(const std::string &normalized)
{
    std::complex<double> probe;
    if(!evaluateComplexPreset(normalized, 0.5, 0.5, probe))
        return false;

    graph_mode = GraphMode::Complex;
    graph_expression = normalized;
    rebuildGraphPoints();
    graph_reveal_x = graph_range;
    graph_line_sweep_enabled = false;
    return true;
}

bool World::setImplicitGraphExpression(const std::string &normalized)
{
    std::vector<double> ys;
    if(!evaluateImplicitPreset(normalized, 0.0, 0.0, ys))
        return false;

    graph_mode = GraphMode::Implicit;
    graph_expression = normalized;
    rebuildGraphPoints();
    graph_reveal_x = graph_range;
    graph_line_sweep_enabled = false;
    return true;
}

bool World::graphPointsAt(int x, int z, const std::vector<GraphPoint> *&out)
{
    const auto key = std::tuple<int,int,int>{x, 0, z};
    auto it = graph_points_by_xz.find(key);
    if(it == graph_points_by_xz.end())
    {
        if(!graph_unbounded && (x < -graph_range || x > graph_range || z < -graph_range || z > graph_range))
            return false;

        std::vector<GraphPoint> sampled;
        sampleGraphColumn(x, z, sampled);
        if(sampled.empty())
            return false;

        it = graph_points_by_xz.emplace(key, sampled).first;
    }

    out = &it->second;
    return true;
}

void World::startGraphLineSweep()
{
    graph_line_sweep_enabled = true;
    graph_reveal_x = -graph_range;
}

void World::stopGraphLineSweep()
{
    graph_line_sweep_enabled = false;
    graph_reveal_x = graph_range;
}

bool World::stepGraphLineSweep()
{
    if(!graph_line_sweep_enabled)
        return false;

    if(graph_reveal_x < graph_range)
    {
        ++graph_reveal_x;
        return true;
    }

    graph_line_sweep_enabled = false;
    return false;
}

void World::rebuildGraphPoints()
{
    graph_points.clear();
    graph_points_by_xz.clear();

    if(graph_unbounded)
        return;

    for(int gx = -graph_range; gx <= graph_range; ++gx)
        for(int gz = -graph_range; gz <= graph_range; ++gz)
        {
            std::vector<GraphPoint> sampled;
            sampleGraphColumn(gx, gz, sampled);
            if(sampled.empty())
                continue;

            auto &bucket = graph_points_by_xz[std::tuple<int,int,int>{gx, 0, gz}];
            for(const GraphPoint &p : sampled)
            {
                graph_points.push_back(p);
                bucket.push_back(p);
            }
        }
}

void World::sampleGraphColumn(int gx, int gz, std::vector<GraphPoint> &out) const
{
    out.clear();

    std::vector<Token> tokens;
    std::vector<Token> rpn;
    const bool real_mode = graph_mode == GraphMode::Real;
    const bool complex_mode = graph_mode == GraphMode::Complex;
    if(real_mode && (!tokenizeGraphExpression(graph_expression, tokens) || !toRpn(tokens, rpn)))
        return;

    constexpr double graph_scale = 8.0;
    const int world_y_max = World::HEIGHT * Chunk::SIZE - 1;

    const double zoom = static_cast<double>(graph_zoom_percent) / 100.0;

    if(real_mode)
    {
        double value = 0.0;
        if(!evaluateRpn(rpn, static_cast<double>(gx) / zoom, static_cast<double>(gz) / zoom, value))
            return;
        const int gy = graph_origin_y + static_cast<int>(std::lround(value * graph_scale));
        if(gy <= 0 || gy >= world_y_max)
            return;
        out.push_back({gx, gy, gz, BLOCK_WOOL_GREEN});
        return;
    }

    if(complex_mode)
    {
        std::complex<double> w;
        if(!evaluateComplexPreset(graph_expression, static_cast<double>(gx) / zoom, static_cast<double>(gz) / zoom, w))
            return;

        const double mag = std::abs(w);
        const double h = std::log1p(mag) * 10.0;
        const int gy = graph_origin_y + static_cast<int>(std::lround(h));
        if(gy <= 0 || gy >= world_y_max)
            return;
        out.push_back({gx, gy, gz, getBLOCKWDATA(phaseToWool(std::arg(w)), 0)});
        return;
    }

    std::vector<double> ys;
    if(!evaluateImplicitPreset(graph_expression, static_cast<double>(gx) / zoom, static_cast<double>(gz) / zoom, ys))
        return;

    for(double yv : ys)
    {
        const int gy = graph_origin_y + static_cast<int>(std::lround(yv * graph_scale));
        if(gy <= 0 || gy >= world_y_max)
            continue;
        out.push_back({gx, gy, gz, BLOCK_WOOL_GREEN});
    }
}

constexpr int getLocal(const int global)
{
     static_assert(Chunk::SIZE == 8, "Update the bit operations accordingly!");

     return global & 0b111;
}

constexpr int getChunk(const int global)
{
    return global >> 3;
}

BLOCK_WDATA World::getBlock(const int x, const int y, const int z) const
{
    int chunk_x = getChunk(x), chunk_y = getChunk(y), chunk_z = getChunk(z);

    Chunk *c = findChunk(chunk_x, chunk_y, chunk_z);
    if(!c)
    {
        //Don't render world edges except for the top
        if(chunk_y == World::HEIGHT)
            return BLOCK_AIR;
        else
            return BLOCK_STONE;
    }

    return c->getLocalBlock(getLocal(x), getLocal(y), getLocal(z));
}

void World::setBlock(const int x, const int y, const int z, const BLOCK_WDATA block, bool set_dirty)
{
    int chunk_x = getChunk(x), chunk_y = getChunk(y), chunk_z = getChunk(z);
    int local_x = getLocal(x), local_y = getLocal(y), local_z = getLocal(z);

    Chunk *c = findChunk(chunk_x, chunk_y, chunk_z);
    if(c)
        c->setLocalBlock(local_x, local_y, local_z, block, set_dirty);
    else
        pending_block_changes.push_back({chunk_x, chunk_y, chunk_z, local_x, local_y, local_z, block});
}

void World::changeBlock(const int x, const int y, const int z, const BLOCK_WDATA block)
{
    int chunk_x = getChunk(x), chunk_y = getChunk(y), chunk_z = getChunk(z);
    int local_x = getLocal(x), local_y = getLocal(y), local_z = getLocal(z);

    Chunk *c = findChunk(chunk_x, chunk_y, chunk_z);
    if(c)
        c->changeLocalBlock(local_x, local_y, local_z, block);
    else
        pending_block_changes.push_back({chunk_x, chunk_y, chunk_z, local_x, local_y, local_z, block});
}

void World::setChunkVisible(int x, int y, int z)
{
    Chunk *c = findChunk(x, y, z);
    if(!c)
        c = generateChunk(x, y, z);

    if(c)
        visible_chunks.push_back(c);
}

void World::setPosition(int x, int y, int z)
{
    int chunk_x = getChunk(positionToBlock(x)),
        chunk_y = getChunk(positionToBlock(y)),
        chunk_z = getChunk(positionToBlock(z));

    chunk_y = std::max(0, std::min(chunk_y, World::HEIGHT - 1));

    if(!loaded || (chunk_x != cen_x || chunk_y != cen_y || chunk_z != cen_z))
    {
        visible_chunks.clear();

        int dist = field_of_view;
        int distsq = dist*dist;

        // Turn the + shape into a cube to load the directly adjacent corners.
        // This avoids worse graphics and bad possibly collision issues.
        if(distsq == 1)
            distsq = 3;

        std::vector<std::tuple<int, int, int>> local_chunks;

        for(int x = -dist; x <= dist; ++x)
            for(int y = -dist; y <= dist; ++y)
                for(int z = -dist; z <= dist; ++z)
                {
                    if(chunk_y + y < 0 || chunk_y + y >= World::HEIGHT)
                        continue;

                    if(x*x + y*y + z*z > distsq)
                        continue;

                    local_chunks.push_back(std::make_tuple(x, y, z));
                }

        std::sort(local_chunks.begin(), local_chunks.end(), [](const std::tuple<int, int, int>& a, const std::tuple<int, int, int>& b) {
            int dist_a = std::get<0>(a)*std::get<0>(a) + std::get<1>(a)*std::get<1>(a) + std::get<2>(a)*std::get<2>(a);
            int dist_b = std::get<0>(b)*std::get<0>(b) + std::get<1>(b)*std::get<1>(b) + std::get<2>(b)*std::get<2>(b);
            return dist_a < dist_b;
        });

        for(const auto& pos : local_chunks)
        {
            setChunkVisible(chunk_x + std::get<0>(pos), chunk_y + std::get<1>(pos), chunk_z + std::get<2>(pos));
        }

        cen_x = chunk_x;
        cen_y = chunk_y;
        cen_z = chunk_z;

        loaded = true;
    }
}

bool World::blockAction(const int x, const int y, const int z)
{
    int chunk_x = getChunk(x), chunk_y = getChunk(y), chunk_z = getChunk(z);
    int local_x = getLocal(x), local_y = getLocal(y), local_z = getLocal(z);

    Chunk *c = findChunk(chunk_x, chunk_y, chunk_z);
    if(!c)
        return false;

    const BLOCK_WDATA block = c->getLocalBlock(local_x, local_y, local_z);
    if(getBLOCK(block) == BLOCK_FURNACE)
    {
        inventory_task.openFurnace(x, y, z);
        return true;
    }
    if(getBLOCK(block) == BLOCK_CRAFTING_TABLE)
    {
        inventory_task.openCraftingTable();
        return true;
    }

    return global_block_renderer.action(block, local_x, local_y, local_z, *c);
}

bool World::intersect(AABB &other) const
{
    for(Chunk *c : visible_chunks)
        if(c->intersects(other))
            return true;

    return false;
}

bool World::intersectsRay(GLFix x, GLFix y, GLFix z, GLFix dx, GLFix dy, GLFix dz, VECTOR3 &result, AABB::SIDE &side, GLFix &dist, bool ignore_water) const
{
    dist = GLFix::maxValue();
    VECTOR3 pos;
    for(Chunk *c : visible_chunks)
    {
        GLFix new_dist;
        AABB::SIDE new_side = AABB::NONE;

        if(c->intersectsRay(x, y, z, dx, dy, dz, new_dist, pos, new_side, ignore_water))
        {
            if(new_dist > dist)
                continue;

            result.x = pos.x + c->x*Chunk::SIZE;
            result.y = pos.y + c->y*Chunk::SIZE;
            result.z = pos.z + c->z*Chunk::SIZE;
            side = new_side;
            dist = new_dist;
        }
    }

    return dist != GLFix::maxValue();
}

const PerlinNoise &World::noiseGenerator() const
{
    return perlin_noise;
}

void World::clear()
{
    for(auto &&c : all_chunks)
        delete c.second;

    all_chunks.clear();
    visible_chunks.clear();
    build_queue.clear();
    pending_block_changes.clear();

    loaded = false;
}

void World::setDirty()
{
    for(auto &&c : all_chunks)
        c.second->setDirty();
}

#define LOAD_FROM_FILE(var) if(gzfread(&(var), sizeof(var), 1, file) != 1) return false;
#define SAVE_TO_FILE(var) if(gzfwrite(&(var), sizeof(var), 1, file) != 1) return false;

bool World::loadFromFile(gzFile file)
{
    drawLoadingtext(1);

    clear();

    LOAD_FROM_FILE(seed)
    perlin_noise.setSeed(seed);

    unsigned int block_changes;
    LOAD_FROM_FILE(block_changes)
    pending_block_changes.resize(block_changes);

    if(gzfread(pending_block_changes.data(), sizeof(BLOCK_CHANGE), block_changes, file) != block_changes)
        return false;

    LOAD_FROM_FILE(field_of_view);

    for(;;)
    {
        int x, y, z;
        if(gzfread(&x, sizeof(x), 1, file) != 1)
            return gzeof(file);

        LOAD_FROM_FILE(y)
        LOAD_FROM_FILE(z)

        Chunk *c = new Chunk(x, y, z);
        if(!c->loadFromFile(file))
        {
            delete c;
            return false;
        }
        all_chunks.insert({std::tuple<int,int,int>(x, y, z), c});
    }
}

bool World::saveToFile(gzFile file) const
{
    drawLoadingtext(1);

    SAVE_TO_FILE(seed)

    unsigned int block_changes = pending_block_changes.size();
    SAVE_TO_FILE(block_changes)

    if(gzfwrite(pending_block_changes.data(), sizeof(BLOCK_CHANGE), block_changes, file) != block_changes)
        return false;

    SAVE_TO_FILE(field_of_view);

    for(auto&& c : all_chunks)
    {
        SAVE_TO_FILE(c.second->x)
        SAVE_TO_FILE(c.second->y)
        SAVE_TO_FILE(c.second->z)

        if(!c.second->saveToFile(file))
            return false;
    }

    return true;
}

void World::processBuildQueue()
{
    // Add dirty chunks to the build queue (avoiding duplicates)
    for(Chunk *c : visible_chunks)
    {
        if(c->isBuildDirty())
        {
            // Check if already in queue
            bool already_queued = false;
            for(Chunk *q : build_queue)
            {
                if(q == c)
                {
                    already_queued = true;
                    break;
                }
            }

            if(!already_queued)
                build_queue.push_back(c);
        }
    }

    // Process one chunk from the queue
    if(!build_queue.empty())
    {
        build_queue.sort([this](Chunk *a, Chunk *b) {
            int dist_a = (a->x - cen_x)*(a->x - cen_x) + (a->y - cen_y)*(a->y - cen_y) + (a->z - cen_z)*(a->z - cen_z);
            int dist_b = (b->x - cen_x)*(b->x - cen_x) + (b->y - cen_y)*(b->y - cen_y) + (b->z - cen_z)*(b->z - cen_z);
            return dist_a < dist_b;
        });

        Chunk *c = build_queue.front();
        build_queue.pop_front();
        c->buildGeometryAsync();
    }
}

void World::render()
{
    bool ticks_enabled = settings_task.getValue(SettingsTask::TICKS_ENABLED);
    for(Chunk *c : visible_chunks)
        c->logic(ticks_enabled);

    processBuildQueue();

    for(Chunk *c : visible_chunks)
        c->render();
}

Chunk* World::findChunk(int x, int y, int z) const
{
    auto&& c = all_chunks.find(std::tuple<int,int,int>{x, y, z});
    if(c == all_chunks.end())
        return nullptr;
    else
        return c->second;
}

void World::spawnDestructionParticles(int x, int y, int z)
{
    auto *c = findChunk(getChunk(x), getChunk(y), getChunk(z));
    int cx = getLocal(x), cy = getLocal(y), cz = getLocal(z);
    return c->spawnDestructionParticles(cx, cy, cz);
}

void World::explosionTNT(int gx, int gy, int gz)
{
    spawnDestructionParticles(gx, gy, gz);
    changeBlock(gx, gy, gz, BLOCK_AIR);

    const int dist = 3;
    for(int ox = -dist; ox <= dist; ++ox)
        for(int oy = -dist; oy <= dist; ++oy)
            for(int oz = -dist; oz <= dist; ++oz)
            {
                if(gy + oy < 1 || gy + oy >= World::HEIGHT * Chunk::SIZE)
                    continue;
                if(ox * ox + oy * oy + oz * oz > dist * dist)
                    continue;

                const int nx = gx + ox, ny = gy + oy, nz = gz + oz;
                const BLOCK block = getBLOCK(getBlock(nx, ny, nz));
                if(block == BLOCK_TNT)
                    explosionTNT(nx, ny, nz);
                else if(block != BLOCK_BEDROCK && block != BLOCK_AIR)
                {
                    spawnDestructionParticles(nx, ny, nz);
                    changeBlock(nx, ny, nz, BLOCK_AIR);
                }
            }
}

Chunk* World::generateChunk(int x, int y, int z)
{
    if(Chunk *c = findChunk(x - 1, y, z))
        c->setDirty();
    if(Chunk *c = findChunk(x + 1, y, z))
        c->setDirty();
    if(Chunk *c = findChunk(x, y - 1, z))
        c->setDirty();
    if(Chunk *c = findChunk(x, y + 1, z))
        c->setDirty();
    if(Chunk *c = findChunk(x, y, z - 1))
        c->setDirty();
    if(Chunk *c = findChunk(x, y, z + 1))
        c->setDirty();

    Chunk *c = new Chunk(x, y, z);
    if(c == nullptr)
        return c;

    c->generate();
    all_chunks.insert({std::tuple<int,int,int>(x, y, z), c});

    for(auto it = pending_block_changes.begin(); it != pending_block_changes.end();)
    {
        BLOCK_CHANGE &block_change = *it;
        if(c->x == block_change.chunk_x && c->y == block_change.chunk_y && c->z == block_change.chunk_z)
        {
            c->setLocalBlock(block_change.local_x, block_change.local_y, block_change.local_z, block_change.block);
            it = pending_block_changes.erase(it);
        }
        else
            ++it;
    }

    return c;
}


void World::setFieldOfView(int fov) {
    field_of_view = fov;
    loaded = false;
    
    int max_dist = fov * Chunk::SIZE * BLOCK_SIZE;
    int fog_max_dist = max_dist - (4 * BLOCK_SIZE);
    if (fog_max_dist < BLOCK_SIZE) fog_max_dist = BLOCK_SIZE;
    
    ngl_fog_start = fog_max_dist / 2;
    int range = fog_max_dist - ngl_fog_start;
    if (range < 1) range = 1;
    ngl_fog_multiplier = (256 << 12) / range;
}

