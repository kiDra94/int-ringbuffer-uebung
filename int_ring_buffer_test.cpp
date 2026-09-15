#include "int_ring_buffer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>


TEST_CASE("Creation", "[new]") {
  ds::IntRingBuffer rb{3};

  SECTION("New buffer is empty (has 0 elements)") {
    CHECK(rb.size() == 0);
    CHECK(rb.empty());
    CHECK(not rb.full());
    CHECK(rb.capacity() == 3);
  }

  SECTION("Front, back, at and pop throw for empty buffer") {
    CHECK_THROWS(rb.front());
    CHECK_THROWS(rb.back());
    CHECK_THROWS(rb.at(0));
    CHECK_THROWS(rb.pop());
  }

  SECTION("Capacity 0 is not allowed") {
    CHECK_THROWS(ds::IntRingBuffer{0});
  }

  SECTION("Buffer with capacity 1") {
    ds::IntRingBuffer one{1};
    one.push(5);
    CHECK(one.full());
    CHECK(one.front() == 5);
    CHECK(one.back() == 5);
    CHECK(one.pop() == 5);
    CHECK(one.empty());
  }
}


TEST_CASE("Add elements", "[push]") {
  ds::IntRingBuffer rb{3};

  SECTION("Add a single element") {
    rb.push(27);
    CHECK(rb.size() == 1);
    CHECK(not rb.empty());
    CHECK(not rb.full());
    CHECK(rb.front() == 27);
    CHECK(rb.back() == 27);
  }

  SECTION("Fill the buffer") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    CHECK(rb.size() == 3);
    CHECK(rb.full());
    CHECK(rb.front() == 1);
    CHECK(rb.back() == 3);
  }

  SECTION("Push to a full buffer throws") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    CHECK_THROWS(rb.push(4));
    CHECK(rb.size() == 3);
    CHECK(rb.front() == 1);
    CHECK(rb.back() == 3);
  }
}


TEST_CASE("Remove elements", "[pop]") {
  ds::IntRingBuffer rb{3};
  rb.push(1);
  rb.push(2);
  rb.push(3);

  SECTION("Elements are removed in FIFO order") {
    CHECK(rb.pop() == 1);
    CHECK(rb.pop() == 2);
    CHECK(rb.pop() == 3);
    CHECK(rb.empty());
  }

  SECTION("Pop frees a slot") {
    rb.pop();
    CHECK(not rb.full());
    CHECK(rb.size() == 2);
    CHECK(rb.front() == 2);
    CHECK_NOTHROW(rb.push(4));
    CHECK(rb.full());
  }

  SECTION("Try to remove from empty buffer") {
    rb.pop();
    rb.pop();
    rb.pop();
    CHECK_THROWS(rb.pop());
    CHECK_THROWS(rb.front());
    CHECK_THROWS(rb.back());
  }
}


TEST_CASE("Wrap-around", "[wrap]") {
  ds::IntRingBuffer rb{4};

  SECTION("Example from the README") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.pop();
    rb.pop();
    rb.push(4);
    rb.push(5);
    CHECK(rb.size() == 3);
    CHECK(rb.front() == 3);
    CHECK(rb.back() == 5);
    CHECK(rb.at(0) == 3);
    CHECK(rb.at(1) == 4);
    CHECK(rb.at(2) == 5);
  }

  SECTION("Fill the buffer after wrap-around") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.pop();
    rb.pop();
    rb.push(4);
    rb.push(5);
    rb.push(6);
    CHECK(rb.full());
    CHECK_THROWS(rb.push(7));
    CHECK(rb.pop() == 3);
    CHECK(rb.pop() == 4);
    CHECK(rb.pop() == 5);
    CHECK(rb.pop() == 6);
    CHECK(rb.empty());
  }

  SECTION("Many rounds through the array") {
    for (int64_t i(0); i < 100; ++i) {
      rb.push(i);
      rb.push(i + 1000);
      CHECK(rb.pop() == i);
      CHECK(rb.front() == i + 1000);
      CHECK(rb.pop() == i + 1000);
    }
    CHECK(rb.empty());
  }

  SECTION("FIFO order is kept over many rounds") {
    rb.push(0);
    rb.push(1);
    rb.push(2);
    for (int64_t i(3); i < 50; ++i) {
      CHECK(rb.pop() == i - 3);
      rb.push(i);
      CHECK(rb.front() == i - 2);
      CHECK(rb.back() == i);
      CHECK(rb.size() == 3);
    }
  }
}


TEST_CASE("Overwrite oldest element", "[overwrite]") {
  ds::IntRingBuffer rb{3};

  SECTION("Behaves like push if the buffer is not full") {
    rb.push_overwrite(1);
    rb.push_overwrite(2);
    CHECK(rb.size() == 2);
    CHECK(rb.front() == 1);
    CHECK(rb.back() == 2);
  }

  SECTION("Overwrites the oldest element if the buffer is full") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    CHECK_NOTHROW(rb.push_overwrite(4));
    CHECK(rb.size() == 3);
    CHECK(rb.full());
    CHECK(rb.front() == 2);
    CHECK(rb.back() == 4);
    CHECK(rb.count(1) == 0);
  }

  SECTION("Only the newest elements are kept") {
    for (int64_t i(1); i <= 10; ++i) {
      rb.push_overwrite(i);
    }
    CHECK(rb.size() == 3);
    CHECK(rb.at(0) == 8);
    CHECK(rb.at(1) == 9);
    CHECK(rb.at(2) == 10);
    CHECK(rb.pop() == 8);
  }
}


TEST_CASE("Element access", "[at]") {
  ds::IntRingBuffer rb{5};
  rb.push(10);
  rb.push(20);
  rb.push(30);

  SECTION("Positions are relative to the oldest element") {
    CHECK(rb.at(0) == 10);
    CHECK(rb.at(1) == 20);
    CHECK(rb.at(2) == 30);
    rb.pop();
    CHECK(rb.at(0) == 20);
    CHECK(rb.at(1) == 30);
  }

  SECTION("At throws only if out of bounds") {
    CHECK_NOTHROW(rb.at(0));
    CHECK_NOTHROW(rb.at(2));
    CHECK_THROWS(rb.at(3));
    CHECK_THROWS(rb.at(4));
    CHECK_THROWS(rb.at(100));
  }
}


TEST_CASE("Count number of matches", "[count]") {
  ds::IntRingBuffer rb{4};

  SECTION("No matches in empty buffer") {
    CHECK(rb.count(0) == 0);
  }

  SECTION("Removed elements are not counted") {
    rb.push(7);
    rb.push(7);
    rb.push(1);
    rb.pop();
    CHECK(rb.count(7) == 1);
    CHECK(rb.count(1) == 1);
    CHECK(rb.count(0) == 0);
  }

  SECTION("Correct matches after wrap-around") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.pop();
    rb.pop();
    rb.push(2);
    rb.push(2);
    rb.push(3);
    CHECK(rb.count(1) == 0);
    CHECK(rb.count(2) == 2);
    CHECK(rb.count(3) == 2);
  }
}


TEST_CASE("Clear the buffer", "[clear]") {
  ds::IntRingBuffer rb{3};
  rb.push(1);
  rb.push(2);
  rb.push(3);

  SECTION("Buffer is empty after clear") {
    rb.clear();
    CHECK(rb.empty());
    CHECK(rb.size() == 0);
    CHECK(rb.capacity() == 3);
    CHECK_THROWS(rb.front());
    CHECK(rb.count(1) == 0);
  }

  SECTION("Buffer can be filled again after clear") {
    rb.clear();
    rb.push(4);
    rb.push(5);
    rb.push(6);
    CHECK(rb.full());
    CHECK(rb.front() == 4);
    CHECK(rb.back() == 6);
  }
}


TEST_CASE("Copy", "[copy]") {
  ds::IntRingBuffer rb{4};
  rb.push(1);
  rb.push(2);
  rb.push(3);
  rb.pop();
  rb.pop();
  rb.push(4);
  rb.push(5);  // wrapped around, content: rb[3, 4, 5]

  SECTION("Copy constructor copies capacity and elements in order") {
    ds::IntRingBuffer other(rb);
    CHECK(other.capacity() == 4);
    CHECK(other.size() == 3);
    CHECK(other.at(0) == 3);
    CHECK(other.at(1) == 4);
    CHECK(other.at(2) == 5);
  }

  SECTION("Copy constructor creates an independent copy") {
    ds::IntRingBuffer other(rb);
    other.pop();
    other.push(6);
    other.push(7);
    CHECK(other.full());
    CHECK(rb.size() == 3);
    CHECK(rb.front() == 3);
    CHECK(rb.back() == 5);
    rb.push(8);
    CHECK(other.back() == 7);
  }

  SECTION("Copy keeps working after wrap-around") {
    ds::IntRingBuffer other(rb);
    other.push(6);
    CHECK(other.full());
    CHECK(other.pop() == 3);
    CHECK(other.pop() == 4);
    CHECK(other.pop() == 5);
    CHECK(other.pop() == 6);
    CHECK(other.empty());
  }

  SECTION("Copy assignment copies capacity and elements") {
    ds::IntRingBuffer other{2};
    other.push(99);
    other = rb;
    CHECK(other.capacity() == 4);
    CHECK(other.size() == 3);
    CHECK(other.front() == 3);
    CHECK(other.back() == 5);
    CHECK(other.count(99) == 0);
    other.push(6);
    CHECK(other.full());
    CHECK(not rb.full());
  }

  SECTION("Copy assignment to a buffer with larger capacity") {
    ds::IntRingBuffer other{10};
    other = rb;
    CHECK(other.capacity() == 4);
    other.push(6);
    CHECK_THROWS(other.push(7));
  }

  SECTION("Self-assignment") {
    ds::IntRingBuffer& same = rb;
    rb = same;
    CHECK(rb.size() == 3);
    CHECK(rb.at(0) == 3);
    CHECK(rb.at(2) == 5);
  }

  SECTION("Chained assignment") {
    ds::IntRingBuffer a{1};
    ds::IntRingBuffer b{1};
    a = b = rb;
    CHECK(a.size() == 3);
    CHECK(b.size() == 3);
    a.pop();
    CHECK(b.front() == 3);
  }
}


TEST_CASE("Print buffer", "[ostream]") {
  ds::IntRingBuffer rb{4};

  SECTION("Print empty buffer") {
    std::stringstream ss;
    ss << rb;
    CHECK(ss.str() == "rb[]");
  }

  SECTION("Print from oldest to newest after wrap-around") {
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.pop();
    rb.pop();
    rb.push(4);
    rb.push(5);
    std::stringstream ss;
    ss << rb;
    CHECK(ss.str() == "rb[3, 4, 5]");
  }

  SECTION("Operator returns the stream") {
    rb.push(-1);
    std::stringstream ss;
    ss << rb << " " << rb;
    CHECK(ss.str() == "rb[-1] rb[-1]");
  }
}


TEST_CASE("Const correctness", "[const]") {
  ds::IntRingBuffer rb{3};
  rb.push(1);
  rb.push(2);
  rb.push(2);
  const ds::IntRingBuffer& c = rb;

  SECTION("Read-only member functions are const") {
    CHECK(c.front() == 1);
    CHECK(c.back() == 2);
    CHECK(c.size() == 3);
    CHECK(c.capacity() == 3);
    CHECK(not c.empty());
    CHECK(c.full());
    CHECK(c.at(1) == 2);
    CHECK(c.count(2) == 2);
  }

  SECTION("Copy from const buffer") {
    ds::IntRingBuffer other(c);
    CHECK(other.size() == 3);
  }
}


TEST_CASE("Performance", "[performance]") {
  const uint64_t capacity = 100000;
  ds::IntRingBuffer rb{capacity};
  for (uint64_t i(0); i < capacity; ++i) {
    rb.push(static_cast<int64_t>(i));
  }

  SECTION("Remove and add 10,000,000 elements in a full buffer") {
    for (int64_t i(0); i < 10000000; ++i) {
      rb.pop();
      rb.push(i);
    }
    CHECK(rb.full());
    CHECK(rb.front() == 10000000 - 100000);
    CHECK(rb.back() == 9999999);
  }

  SECTION("Overwrite 10,000,000 elements") {
    for (int64_t i(0); i < 10000000; ++i) {
      rb.push_overwrite(i);
    }
    CHECK(rb.size() == capacity);
    CHECK(rb.front() == 10000000 - 100000);
    CHECK(rb.back() == 9999999);
  }
}
