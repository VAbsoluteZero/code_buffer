
 // #todo compare compile time of naive approach w/ assign_helper
 		template <typename U, typename T, int... Number>
		struct AssignHelper;

		template <int... Number>
		struct AssignHelper<Tuple<Types...>, std::integer_sequence<int, Number...>>
		{
			template <typename... Other>
			static constexpr void SetValue(Tuple<Types...>& Self, const Tuple<Other...>& val) noexcept
			{
				(..., (((ValueHolder<Number, Types>&)(Self)) = ((const ValueHolder<Number, Other>&)(val)).Value));
			}
		};
		template <typename... Others>
		void AssignMembers(const Tuple<Others...>& val)
		{
			constexpr auto sz = (sizeof...(Types));
			if constexpr (sz > 0)
				this->template get<0>() = val.template get<0>();

			if constexpr (sz > 1)
				this->template get<1>() = val.template get<1>();

			if constexpr (sz > 2)
				this->template get<2>() = val.template get<2>();

			if constexpr (sz > 3)
				this->template get<3>() = val.template get<3>();

			if constexpr (sz > 4)
				this->template get<4>() = val.template get<4>();

			if constexpr (sz > 5)
				this->template get<5>() = val.template get<5>();

			if constexpr (sz > 6)
				this->template get<6>() = val.template get<6>();

			if constexpr (sz > 7)
				this->template get<7>() = val.template get<7>();

			if constexpr (sz > 8)
				this->template get<8>() = val.template get<8>();

			if constexpr (sz > 9)
				this->template get<9>() = val.template get<9>();

			if constexpr (sz > 10)
				this->template get<10>() = val.template get<10>();

			if constexpr (sz > 11)
				this->template get<10>() = val.template get<11>();

			if constexpr (sz > 12)
				static_assert(false, "too many types");
		}
