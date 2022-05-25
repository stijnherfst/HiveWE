module;

#include <memory>

export module no_init_allocator;

export template <typename T, typename A = std::allocator<T>>
class default_init_allocator : public A {
	typedef std::allocator_traits<A> a_t;

  public:
	using A::A; // Inherit constructors from A

	template <typename U>
	struct rebind {
		using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
	};

	template <typename U>
	void construct(U* ptr) noexcept(std::is_nothrow_default_constructible<U>::value) {
		::new (static_cast<void*>(ptr)) U;
	}

	template <typename U, typename... Args>
	void construct(U* ptr, Args&&... args) {
		a_t::construct(static_cast<A&>(*this),
					   ptr, std::forward<Args>(args)...);
	}
};