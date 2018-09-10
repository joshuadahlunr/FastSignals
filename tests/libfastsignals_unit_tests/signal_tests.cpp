#include "catch2/catch.hpp"
#include "libfastsignals/include/signal.h"
#include <string>

using namespace is::signals;
using namespace std::literals;

namespace
{
template <class T>
class any_of_combiner
{
public:
	static_assert(std::is_same_v<T, bool>);

	using result_type = bool;

	template <class TRef>
	void operator()(TRef&& value)
	{
		m_result = m_result || bool(value);
	}

	result_type get_value() const
	{
		return m_result;
	}

private:
	result_type m_result = {};
};
} // namespace

TEST_CASE("Can connect a few slots and emit", "[signal]")
{
	signal<void(int)> valueChanged;

	int value1 = 0;
	int value2 = 0;
	valueChanged.connect([&value1](int value) {
		value1 = value;
	});
	valueChanged.connect([&value2](int value) {
		value2 = value;
	});
	REQUIRE(value1 == 0);
	REQUIRE(value2 == 0);

	valueChanged(10);
	REQUIRE(value1 == 10);
	REQUIRE(value2 == 10);
}

TEST_CASE("Can safely pass rvalues", "[signal]")
{
	const std::string expected = "If the type T is a reference type, provides the member typedef type which is the type referred to by T. Otherwise type is T.";
	std::string passedValue = expected;
	signal<void(std::string)> valueChanged;

	std::string value1;
	std::string value2;
	valueChanged.connect([&value1](std::string value) {
		value1 = value;
	});
	valueChanged.connect([&value2](std::string value) {
		value2 = value;
	});

	valueChanged(std::move(passedValue));
	REQUIRE(value1 == expected);
	REQUIRE(value2 == expected);
}

TEST_CASE("Can pass mutable ref", "[signal]")
{
	const std::string expected = "If the type T is a reference type, provides the member typedef type which is the type referred to by T. Otherwise type is T.";
	signal<void(std::string&)> valueChanged;

	std::string passedValue;
	valueChanged.connect([expected](std::string& value) {
		value = expected;
	});
	valueChanged(passedValue);

	REQUIRE(passedValue == expected);
}

TEST_CASE("Can disconnect slot with explicit call", "[signal]")
{
	signal<void(int)> valueChanged;

	int value1 = 0;
	int value2 = 0;
	int value3 = 0;
	auto conn1 = valueChanged.connect([&value1](int value) {
		value1 = value;
	});
	auto conn2 = valueChanged.connect([&value2](int value) {
		value2 = value;
	});
	valueChanged.connect([&value3](int value) {
		value3 = value;
	});
	REQUIRE(value1 == 0);
	REQUIRE(value2 == 0);
	REQUIRE(value3 == 0);

	valueChanged(10);
	REQUIRE(value1 == 10);
	REQUIRE(value2 == 10);
	REQUIRE(value3 == 10);

	conn2.disconnect();
	valueChanged(-99);
	REQUIRE(value1 == -99);
	REQUIRE(value2 == 10);
	REQUIRE(value3 == -99);

	conn1.disconnect();
	valueChanged(17);
	REQUIRE(value1 == -99);
	REQUIRE(value2 == 10);
	REQUIRE(value3 == 17);
}

TEST_CASE("Can disconnect slot with scoped_connection", "[signal]")
{
	signal<void(int)> valueChanged;

	int value1 = 0;
	int value2 = 0;
	int value3 = 0;
	{
		scoped_connection conn1 = valueChanged.connect([&value1](int value) {
			value1 = value;
		});
		{
			scoped_connection conn2 = valueChanged.connect([&value2](int value) {
				value2 = value;
			});
			valueChanged.connect([&value3](int value) {
				value3 = value;
			});
			REQUIRE(value1 == 0);
			REQUIRE(value2 == 0);
			REQUIRE(value3 == 0);

			valueChanged(10);
			REQUIRE(value1 == 10);
			REQUIRE(value2 == 10);
			REQUIRE(value3 == 10);
		}

		// conn2 disconnected.
		valueChanged(-99);
		REQUIRE(value1 == -99);
		REQUIRE(value2 == 10);
		REQUIRE(value3 == -99);
	}

	// conn1 disconnected.
	valueChanged(17);
	REQUIRE(value1 == -99);
	REQUIRE(value2 == 10);
	REQUIRE(value3 == 17);
}

TEST_CASE("Can disconnect all", "[signal]")
{
	signal<void(int)> valueChanged;

	int value1 = 0;
	int value2 = 0;
	int value3 = 0;
	valueChanged.connect([&value1](int value) {
		value1 = value;
	});
	valueChanged.connect([&value2](int value) {
		value2 = value;
	});
	valueChanged.connect([&value3](int value) {
		value3 = value;
	});
	REQUIRE(value1 == 0);
	REQUIRE(value2 == 0);
	REQUIRE(value3 == 0);

	valueChanged(63);
	REQUIRE(value1 == 63);
	REQUIRE(value2 == 63);
	REQUIRE(value3 == 63);

	valueChanged.disconnect_all_slots();
	valueChanged(101);
	REQUIRE(value1 == 63);
	REQUIRE(value2 == 63);
	REQUIRE(value3 == 63);
}

TEST_CASE("Can disconnect inside slot", "[signal]")
{
	signal<void(int)> valueChanged;

	int value1 = 0;
	int value2 = 0;
	int value3 = 0;
	connection conn2;
	valueChanged.connect([&value1](int value) {
		value1 = value;
	});
	conn2 = valueChanged.connect([&](int value) {
		value2 = value;
		conn2.disconnect();
	});
	valueChanged.connect([&value3](int value) {
		value3 = value;
	});
	REQUIRE(value1 == 0);
	REQUIRE(value2 == 0);
	REQUIRE(value3 == 0);

	valueChanged(63);
	REQUIRE(value1 == 63);
	REQUIRE(value2 == 63);
	REQUIRE(value3 == 63);

	valueChanged(101);
	REQUIRE(value1 == 101);
	REQUIRE(value2 == 63); // disconnected in slot.
	REQUIRE(value3 == 101);
}

TEST_CASE("Disconnects OK if signal dead first", "[signal]")
{
	connection conn2;
	{
		scoped_connection conn1;
		{
			signal<void(int)> valueChanged;
			conn2 = valueChanged.connect([](int) {
			});
			// Just unused.
			valueChanged.connect([](int) {
			});
			conn1 = valueChanged.connect([](int) {
			});
		}
		REQUIRE(conn2.connected());
		REQUIRE(conn1.connected());
		conn2.disconnect();
		REQUIRE(!conn2.connected());
		REQUIRE(conn1.connected());
	}
	conn2.disconnect();
}

TEST_CASE("Returns last called slot result with default combiner", "[signal]")
{
	connection conn2;
	{
		scoped_connection conn1;
		{
			signal<int(int)> absSignal;
			conn2 = absSignal.connect([](int value) {
				return value * value;
			});
			conn1 = absSignal.connect([](int value) {
				return abs(value);
			});
			absSignal(-1);

			REQUIRE(absSignal(45) == 45);
			REQUIRE(absSignal(-1) == 1);
			REQUIRE(absSignal(-177) == 177);
			REQUIRE(absSignal(0) == 0);
		}
		REQUIRE(conn2.connected());
		conn2.disconnect();
		REQUIRE(!conn2.connected());
	}
	conn2.disconnect();
	REQUIRE(!conn2.connected());
}

TEST_CASE("Works with custom any_of combiner", "[signal]")
{
	using cancellable_signal = signal<bool(std::string), any_of_combiner>;
	cancellable_signal startRequested;
	auto conn1 = startRequested.connect([](std::string op) {
		return op == "1";
	});
	auto conn2 = startRequested.connect([](std::string op) {
		return op == "1" || op == "2";
	});
	REQUIRE(startRequested("0") == false);
	REQUIRE(startRequested("1") == true);
	REQUIRE(startRequested("2") == true);
	REQUIRE(startRequested("3") == false);
	conn1.disconnect();
	conn2.disconnect();
	REQUIRE(startRequested("0") == false);
	REQUIRE(startRequested("1") == false);
	REQUIRE(startRequested("2") == false);
	REQUIRE(startRequested("3") == false);
}

TEST_CASE("Can release scoped connection", "[signal]")
{
	int value2 = 0;
	int value3 = 0;
	signal<void(int)> valueChanged;
	connection conn1;
	{
		scoped_connection conn2;
		scoped_connection conn3;
		conn2 = valueChanged.connect([&value2](int x) {
			value2 = x;
		});
		conn3 = valueChanged.connect([&value3](int x) {
			value3 = x;
		});

		valueChanged(42);
		REQUIRE(value2 == 42);
		REQUIRE(value3 == 42);
		REQUIRE(conn2.connected());
		REQUIRE(conn3.connected());
		REQUIRE(!conn1.connected());

		conn1 = conn3.release();
		REQUIRE(conn2.connected());
		REQUIRE(!conn3.connected());
		REQUIRE(conn1.connected());
		valueChanged(144);
		REQUIRE(value2 == 144);
		REQUIRE(value3 == 144);
	}

	// conn2 disconnected, conn1 connected.
	valueChanged(17);
	REQUIRE(value2 == 144);
	REQUIRE(value3 == 17);
	REQUIRE(conn1.connected());

	conn1.disconnect();
	valueChanged(90);
	REQUIRE(value2 == 144);
	REQUIRE(value3 == 17);
}

TEST_CASE("Can use signal with more than one argument", "[signal]")
{
	signal<void(int, std::string, std::vector<std::string>)> event;

	int value1 = 0;
	std::string value2;
	std::vector<std::string> value3;
	event.connect([&](int v1, const std::string& v2, const std::vector<std::string>& v3) {
		value1 = v1;
		value2 = v2;
		value3 = v3;
	});

	event(9815, "using namespace std::literals!"s, std::vector{ "std::vector"s, "using namespace std::literals"s });
	REQUIRE(value1 == 9815);
	REQUIRE(value2 == "using namespace std::literals!"s);
	REQUIRE(value3 == std::vector{ "std::vector"s, "using namespace std::literals"s });
}
