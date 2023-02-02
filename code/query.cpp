/*

	Compileer met: 
	g++ -Wall -O2 -std=c++17 -o query query.cpp particle.cpp

*/

#include "particle.h"

#include <cmath>
#include <numeric>

using RealType = double;

template <typename T>
constexpr T sign(T x) {
	if (x > 0) return 1;
	if (x < 0) return -1;
	return 0;
}

struct Vec2 {
	RealType x{}, y{};

	Vec2(RealType x, RealType y) : x(x), y(y) {}
	Vec2() = default;

	RealType dist2() const {
		return x * x + y * y;
	}

	RealType dist() const {
		return std::sqrt(dist2());
	}

	Vec2 operator+(const Vec2 &o) const {
		return {x + o.x, y + o.y};
	}

	Vec2 operator-(const Vec2 &o) const {
		return {x - o.x, y - o.y};
	}
};

struct Line {
	RealType a{}, b{}, c{};

	Line(RealType a, RealType b, RealType c=1): a(a), b(b), c(c) {}
	Line() = default;
	Line(Vec2 p1, Vec2 p2): Line(
			p2.y - p1.y,
			p1.x - p2.x,
			p1.x * p2.y - p2.x * p1.y
		) {}

	RealType dist2() const {
		return a * a + b * b;
	}

	RealType dist() const {
		return std::sqrt(dist2());
	}

	void norm() {
		operator/=(dist());
	}

	RealType angle() const {
		return std::atan(-a / b) / 3.141592 * 180;
	}

	Vec2 parametic_t0() const {
		auto d = dist2();
		return {c * (a + b) / d, c * (b - a) / d};
	}

	Vec2 parametic_offset(RealType t) const {
		return {-b * t, a * t};
	}

	Vec2 parametic(RealType t) const {
		return parametic_offset(t) + parametic_t0();
	}

	RealType projection(const Vec2 &p) const {
		return (p.y * a - p.x * b + c) / dist2();
	}

	Line &operator+=(const Line &o) {
		a += o.a;
		b += o.b;
		c += o.c;
		return *this;
	}

	Line &operator-=(const Line &o) {
		a -= o.a;
		b -= o.b;
		c -= o.c;
		return *this;
	}

	Line &operator*=(RealType x) {
		a *= x;
		b *= x;
		c *= x;
		return *this;
	}

	Line &operator/=(RealType x) {
		return this->operator*=(1 / x);
	}

	Line operator*(RealType x) const {
		return {a*x, b*x, c*x};
	}

	void print() const {
		std::cout << a << "x + " << b << "y = " << c << '\n';
	}
};

struct Pixel {
	Vec2 pos{};
	RealType w{};

	Pixel(Vec2 p, RealType w) : pos(p), w(w) {}
};

std::vector<Pixel> pixels_from_particle(const Particle &p) {
	std::vector<Pixel> result;
	int n = 0;
	for (auto x : p.data) n += x != 0;
	auto avg = RealType(std::accumulate(p.data.begin(), p.data.end(), 0)) / RealType(n);
	for (int i = 0; i < p.width * p.height; ++i) {
		if (p.data[i]) result.emplace_back(Vec2{RealType(i % p.width), RealType(i / p.width)}, RealType(p.data[i]) / avg);
	}
	return result;
}

RealType score(const Line &line, const std::vector<Pixel> &pixels) {
	RealType C = 0;
	for (auto &p : pixels) {
		auto x = line.a * p.pos.x + line.b * p.pos.y - line.c;
		C = std::max(x * x, C);
	}
	return C / RealType(pixels.size()) / line.dist2();
}

Line score_gradient(const Line &l, const Pixel &p, RealType d2) {
	RealType C = (-l.c + l.a * p.pos.x + l.b * p.pos.y);
	return {
		p.w * C * (-l.a * C + p.pos.x * d2),
		p.w * C * (-l.b * C + p.pos.y * d2),
		p.w * -C * d2,
	};
}

std::pair<Line, RealType> get_line_from_pixels(const std::vector<Pixel> &pixels, int iterations=10000, RealType gamma=0.001) {
	Line result(pixels.front().pos, pixels.back().pos);
	result.norm();

	RealType n = RealType(pixels.size());
	for (int i = 0; i < iterations; ++i) {
		Line gradient{};
		auto d2 = result.dist2();

		for (auto &p : pixels) {
			gradient += score_gradient(result, p, d2);
		}

		result -= gradient * (2 * gamma / (n * d2 * d2));
		result.norm();
	}

	return {result, score(result, pixels)};
}

int max_delta(const Particle &p) {
	std::vector<RealType> sorted;
	sorted.reserve(p.data.size());
	for (auto i = 0; i < p.area(); ++i) sorted.push_back(p.data[i]);
	std::sort(sorted.begin(), sorted.end());
	std::vector<int> delta(sorted.size());
	std::adjacent_difference(sorted.begin(), sorted.end(), delta.begin());
	return *std::max_element(delta.begin() + (delta.end() - delta.begin()) / 2, delta.end());
}

RealType avg_energy(const Particle &p) {
	int pixels = 0, total = 0;
	for (auto i = 0; i < p.area(); ++i) {
		if (p.data[i]) ++pixels;
		total += p.data[i];
	}

	return RealType(total) / RealType(pixels);
}

RealType length_segment(const Line &line, const std::vector<Pixel> &p) {
	std::vector<RealType> t(p.size());
	int n = p.size();
	for (int i = 0; i < n; ++i) {
		t[i] = line.projection(p[i].pos);
	}
	std::vector<int> sorted(p.size());
	std::iota(sorted.begin(), sorted.end(), 0);
	std::stable_sort(sorted.begin(), sorted.end(), [&t] (auto a, auto b) {
		return t[a] < t[b];
	});
	return (line.parametic_offset((t[sorted[0]] * 10  * p[sorted[0]].w   + t[sorted[1]]   * p[sorted[1]].w)   / (10 * p[sorted[0]].w   + p[sorted[1]].w))
		  - line.parametic_offset((t[sorted[n-1]] * 10 * p[sorted[n-1]].w + t[sorted[n-2]] * p[sorted[n-2]].w) / (10 * p[sorted[n-1]].w + p[sorted[n-2]].w))).dist();
}

RealType vertical_angle(RealType length) {
	return std::atan(5.48571428571 / length) * 180 / 3.141592;
}

int main(int argc, char const *argv[]) {
	if (!(argc == 2 || argc == 3)) {
		std::cerr << "No arguments given.\n"
		return 0;
	}

	double offset_angle = 0;
	if (argc == 3) offset_angle = std::stod(argv[2]);

/*

	Hieronder staat de query die wordt uitgevoerd op elk deeltje.

	Hier staan alle drempelwaardes in verstopt. Dus verander hier waar nodig.

*/

	auto batch = read_batch_filtered<std::tuple<Particle, Line, RealType, int, RealType, RealType, RealType>>(argv[1], [=](auto &x) {
		auto &[p, line, cost, md, length, h_angle, v_angle] = x;
		if (p.touches_border() || avg_energy(p) >= 80 || *std::max_element(p.data.begin(), p.data.end()) >= 275) return false;

		auto pixels = pixels_from_particle(p);
		if (pixels.size() < 5) return false;

		md = max_delta(p);
		if (md >= 40) return false;

		std::tie(line, cost) = get_line_from_pixels(pixels);
		line.norm();
		length = length_segment(line, pixels);
		h_angle = line.angle() + offset_angle;
		if (h_angle <= -90) h_angle += 180;
		else if (h_angle >= 90) h_angle -= 180;
		v_angle = vertical_angle(length);

/*

	Doe de '//' hieronder weg als je op horizontale of verticale invalshoek wil filteren.

*/
		return cost < .05
			//&& std::abs(v_angle) < 20
			//&& std::abs(std::abs(h_angle) - 90) < 30
		;
	});


	std::stable_sort(batch.begin(), batch.end(), [](const auto &a, const auto &b) {
		return std::get<2>(a) < std::get<2>(b);
	});

	std::cout << "filtered batch size: " << batch.size() << '\n';

/*

	Hieronder staan H_N en V_N. 

	- H_N is het aantal partjes van de horizontale meting van -90 tot 90 graden.
	- V_N is van verticale meting van 0 tot 90 graden.

*/

	constexpr int H_N = 15, V_N = 20;

	RealType h_angle_sum = 0, v_angle_sum = 0;

	int h_count[H_N]{};
	int v_count[V_N]{};
	for (const auto &[p, line, cost, md, length, h_angle, v_angle] : batch) {
		h_angle_sum += h_angle;
		v_angle_sum += v_angle;
		h_count[std::min(int(((h_angle + 90) / 180) * RealType(H_N)), H_N-1)]++;
		v_count[std::min(int((v_angle / 90) * RealType(V_N)), V_N-1)]++;
	}

	std::cout << '\n';

	std::cout << "h_angle:\n";
	for (int i = 0; i < H_N; ++i) std::cout << RealType(i * 180 / H_N - 90) + 90. / RealType(H_N) << "," << RealType(h_count[i]) << '\n';
	std::cout << '\n';

	std::cout << "v_angle:\n";
	for (int i = 0; i < V_N; ++i) {
		auto angle = RealType(i * 90 / V_N) + 45. / RealType(V_N);
		std::cout << angle << ", " << v_count[i] << '\n';
	}
	std::cout << "\n";

	std::cout << "v_angle / sin(theta):\n";
	for (int i = 0; i < V_N; ++i) {
		auto angle = RealType(i * 90 / V_N) + 45. / RealType(V_N);
		std::cout << angle << ", " << RealType(v_count[i]) / std::sin(angle * 3.141592 / 180) << '\n';
	}
	std::cout << "\n";

	std::cout << "avg horizontal angle: " << h_angle_sum / RealType(batch.size()) << '\n';
	std::cout << "avg vertical angle: " << v_angle_sum / RealType(batch.size()) << '\n';
	std::cout << "offset angle: " << offset_angle << '\n';

	write_canvas_to_file(convert_to_canvas(batch, [](const auto &p) {
		return &std::get<0>(p);
		auto angle = std::abs(std::get<5>(p));
		if (angle <= 10 || angle >= 80) return &std::get<0>(p);
		return (const Particle *)nullptr;
	}), "plaatje.txt");

	CANVAS last_p{};
	std::get<0>(batch.back()).imprint_on_canvas(last_p);
	write_canvas_to_file(last_p, "laatste_meting.txt");

	return 0;
}