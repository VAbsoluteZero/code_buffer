#pragma once

#include "CoreTemplates.h"

namespace vex::union_impl
{
	template <typename TSelf, typename... Types>
	struct UnionBase
	{
		static_assert(sizeof...(Types) <= 7, "too many types in Union");
		static constexpr auto SizeOfStorage = vex::memory::MaxSizeOf<Types...>();
		static constexpr auto Alignment = vex::memory::MaxAlignOf<Types...>();
		static constexpr auto TypeCount = sizeof...(Types);

		static constexpr byte kNullVal = vex::traits::kTypeIndexNone;

		template <typename T>
		static constexpr size_t Id()
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return typeIndex;
		}

		bool HasAnyValue() const { return ValueIndex != kNullVal; }

		template <typename T>
		bool Has() const
		{
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			return typeIndex == ValueIndex;
		}

		template <typename T>
		T& GetUnchecked()
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");
#if !NDEBUG
			constexpr auto typeIndex = traits::GetIndex<TArg, Types...>();
			if (typeIndex != this->ValueIndex)
				std::abort();
#endif
			return *(reinterpret_cast<TArg*>(this->Storage));
		}

		template <typename T>
		const T& GetUnchecked() const
		{
			using TArg = std::remove_reference_t<T>;
			static_assert(traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");
#if !NDEBUG
			constexpr auto typeIndex = traits::GetIndex<TArg, Types...>();
			if (typeIndex != this->ValueIndex)
				std::abort();
#endif
			return *(reinterpret_cast<const TArg*>(this->Storage));
		}

		template <typename T>
		T* Find()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<T*>(this->Storage));
		}

		template <typename T>
		const T* Find() const
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!UnionBase::Has<T>())
				return nullptr;

			return (reinterpret_cast<const T*>(this->Storage));
		}

		template <typename TArg>
		inline void Match(void (*Func)(TArg&&))
		{
			using TUnderlying = std::decay_t<TArg>;
			if (!UnionBase::Has<TUnderlying>())
				return;

			Func((this)->GetUnchecked<TUnderlying>());
		}

		template <typename TFunc>
		inline void Match(TFunc Func)
		{
			using TraitsT = traits::FunctorTraits<decltype(Func)>;
			using TArg0 = std::decay_t<typename TraitsT::template ArgTypesT<0>>;

			if (!UnionBase::Has<TArg0>())
				return;

			Func((this)->GetUnchecked<TArg0>());
		}

		template <typename... TFuncs>
		inline void MultiMatch(TFuncs&&... Funcs)
		{
			[](...) {
			}((Match(Funcs), 0)...);
		}

		inline void Reset()
		{
			if (!HasAnyValue())
				return;
			((TSelf*)this)->DestroyValue();
			this->ValueIndex = kNullVal;
		}

		byte TypeIndex() const { return ValueIndex; }

		operator bool() const { return HasAnyValue(); }

	protected:
		byte ValueIndex = kNullVal; // intentionally first, aware of padding
		alignas(Alignment) byte Storage[SizeOfStorage];

		template <typename T>
		inline void SetTypeIndex()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			constexpr auto typeIndex = traits::GetIndex<T, Types...>();
			this->ValueIndex = typeIndex;
		}
	};

	template <bool AreAllTrivial, typename... Types>
	struct UnionImpl;

	template <typename... Types>
	struct UnionImpl<true, Types...> : public UnionBase<UnionImpl<true, Types...>, Types...>
	{
		using Base = UnionBase<UnionImpl<true, Types...>, Types...>;
		friend struct UnionBase<UnionImpl<true, Types...>, Types...>;

		constexpr UnionImpl() = default;
		UnionImpl(const UnionImpl&) = default;
		UnionImpl(UnionImpl&&) = default;

		UnionImpl& operator=(const UnionImpl&) = default;
		UnionImpl& operator=(UnionImpl&&) = default;

		static constexpr bool IsTrivial = true;

		template <typename T>
		UnionImpl(T&& Arg)
		{
			using TUnderlying = std::decay_t<T>;
			static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");

			new (this->Storage) TUnderlying(std::forward<T>(Arg));
			this->template SetTypeIndex<T>();
		}

		template <typename T, typename TArg = T>
		inline void Set(TArg&& Val)
		{
			constexpr bool kConv = std::is_convertible_v<TArg, T>; // (... || std::is_convertible_v<T, Types>);
			static_assert(kConv || traits::HasType<TArg, Types...>(), "Union cannot possibly contain this type");

			if (std::is_same_v<T, TArg>)
			{
				*(reinterpret_cast<T*>(this->Storage)) = std::forward<std::decay_t<TArg>>(Val);
			}
			else // convert
			{
				*(reinterpret_cast<T*>(this->Storage)) = std::forward<TArg>(Val);
			}
			this->template SetTypeIndex<T>();
		}
		template <typename T>
		inline void Set(T&& Val)
		{
			using TUnderlying = std::decay_t<T>;

			static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");

			*(reinterpret_cast<TUnderlying*>(this->Storage)) = std::forward<T>(Val);
			this->template SetTypeIndex<T>();
		}

		//  reutrn default value if empty
		template <typename T>
		inline T GetValueOrDefault(T defaultVal) const
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");

			if (!this->template Has<T>())
				return defaultVal;

			return *(reinterpret_cast<const T*>(this->Storage));
		}

		// ok for primitives lets be like map and just construct the val if it is not there
		template <typename T>
		inline T& Get()
		{
			static_assert(traits::HasType<T, Types...>(), "Union cannot possibly contain this type");
			if (!this->template Has<T>())
				this->Set<T>(T());

			return *(reinterpret_cast<T*>(this->Storage));
		}

	private:
		// not calling dtor since storage restricted to PODs
		inline void DestroyValue() {}
	};


	template <typename... Types>
	struct UnionImpl<false, Types...> : public UnionBase<UnionImpl<false, Types...>, Types...>
	{
		using Base = UnionBase<UnionImpl<false, Types...>, Types...>;
		friend struct UnionBase<UnionImpl<false, Types...>, Types...>;

		using TSelf = UnionImpl<false, Types...>;
		constexpr UnionImpl() = default;

		static constexpr bool IsTrivial = false;

	public:
		UnionImpl(const UnionImpl& other)
		{ //
			static_assert((... & std::is_move_constructible_v<Types>), "Union contains non-movable type");
			if (other.HasAnyValue())
			{
				InvokeCopyCTOR(this, &other);
			}
			this->ValueIndex = other.ValueIndex;
		}

		UnionImpl& operator=(const UnionImpl& other)
		{
			if (this != &other)
			{
				if (!other.HasAnyValue())
				{
					this->Reset();
					return *this;
				}
				if (this->ValueIndex == other.ValueIndex)
					InvokeCopyAssignment(&other);
				else
				{
					this->Reset();
					this->ValueIndex = other.ValueIndex;
					InvokeCopyCTOR(other);
				};
			}

			return *this;
		}

		UnionImpl& operator=(UnionImpl&& other)
		{
			if (this != &other)
			{
				if (!other.HasAnyValue())
				{
					this->Reset();
					return *this;
				}
				// bug - I assume that thi shas save val.
				if (this->ValueIndex == other.ValueIndex)
					InvokeMoveAssignment(&other);
				else
				{
					this->Reset();
					this->ValueIndex = other.ValueIndex;
					InvokeMoveCTOR(&other);
				}
			}

			return *this;
		}
		template <typename T>
		UnionImpl(T&& arg)
		{
			using TUnderlying = std::decay_t<T>;

			if constexpr (std::is_same_v<TUnderlying, TSelf>)
			{
				if (arg.HasAnyValue())
				{
					this->ValueIndex = arg.ValueIndex;
					InvokeMoveCTOR(&arg);
					arg.Reset();
				}
			}
			else
			{
				static_assert(traits::HasType<TUnderlying, Types...>(), "Union cannot possibly contain this type");
				new (this->Storage) TUnderlying(std::forward<T>(arg));
				this->template SetTypeIndex<TUnderlying>();
			};
		}

		~UnionImpl() { this->Reset(); }

		template <typename TargetType, typename TArg = TargetType>
		void Set(TArg&& Val)
		{
			constexpr bool kConv = std::is_convertible_v<TArg, TargetType>; // (... || std::is_convertible_v<T, Types>);
			static_assert(kConv || traits::HasType<TargetType, Types...>(), // ? #todo better check
				"Union cannot possibly contain this type");

			if constexpr (std::is_same_v<TargetType, TArg>)
			{
				if (this->template Has<TargetType>())
					*(reinterpret_cast<TargetType*>(this->Storage)) = std::forward<TargetType>(Val);
				else
				{
					if (this->HasAnyValue())
						this->Reset();
					new (this->Storage) TargetType(std::forward<TargetType>(Val));
				}
			}
			else // convert
			{
				if (this->template Has<TargetType>())
					*(reinterpret_cast<TargetType*>(this->Storage)) = std::forward<TArg>(Val);
				else
				{
					if (this->HasAnyValue())
						this->Reset();
					new (this->Storage) TargetType(std::forward<TArg>(Val));
				}
			}
			this->template SetTypeIndex<TargetType>();
		}

		template <typename T>
		void SetDefault()
		{
			using TArg = std::decay_t<T>;
			if (this->template HasAnyValue())
				this->Reset();
			new (this->Storage) TArg();
			this->template SetTypeIndex<TArg>();
		}

	private:
		// table of ctor/dtor's to be resolved based on type index
		struct MethodTable
		{
			void (*CopyConstruct)(TSelf* self, const TSelf* other);
			void (*CopyAssignment)(TSelf* self, const TSelf* other);
			void (*MoveConstruct)(TSelf* self, TSelf* other);
			void (*MoveAssignment)(TSelf* self, TSelf* other);
			void (*Destructor)(TSelf* self);
		};

		template <typename T>
		static void CopyConstructor(TSelf* self, const TSelf* other)
		{ //
			new (self->Storage) T(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void MoveConstructor(TSelf* self, TSelf* other)
		{ //
			new (self->Storage) T(std::move(other->template GetUnchecked<T>()));
			other->Reset();
		}

		template <typename T>
		static void CopyAssignment(TSelf* self, const TSelf* other)
		{ //
			self->template GetUnchecked<T>() = other->template GetUnchecked<T>();
		}

		template <typename T>
		static void MoveAssignment(TSelf* self, TSelf* other)
		{ //
			self->template GetUnchecked<T>() = std::move(other->template GetUnchecked<T>());
		}

		template <typename T>
		static void Destructor(TSelf* self)
		{ //
			auto* value = reinterpret_cast<T*>(self->Storage);
			value->~T();
		}

		template <typename T>
		static constexpr MethodTable BuildMethodTable()
		{ //
			auto table = MethodTable();

			table.Destructor = &TSelf::Destructor<T>;

			table.MoveConstruct = &TSelf::MoveConstructor<T>;
			table.CopyConstruct = &TSelf::CopyConstructor<T>;

			table.CopyAssignment = &TSelf::CopyAssignment<T>;
			table.MoveAssignment = &TSelf::MoveAssignment<T>;

			return table;
		}

		// maps type to method table
		static inline MethodTable gTables[sizeof...(Types)] = {BuildMethodTable<Types>()...};


		MethodTable* Resolve()
		{
			assert(Base::HasAnyValue());
			u32 index = this->ValueIndex;
			return &gTables[index];
		}

		void DestroyValue()
		{
			// this ptr is source of type info
			MethodTable* table = this->Resolve();
			table->Destructor(this);
		}

		void InvokeCopyCTOR(const TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->CopyConstruct(this, other);
		}
		void InvokeCopyAssignment(const TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->CopyConstruct(this, other);
		}

		void InvokeMoveCTOR(TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->MoveConstruct(this, other);
		}
		void InvokeMoveAssignment(TSelf* other)
		{
			// other is source of type info
			MethodTable* table = other->Resolve();
			table->MoveAssignment(this, other);
		}
	};
} // namespace vex::union_impl

namespace vex
{
	template <typename... Types>
	using Union = vex::union_impl::UnionImpl<vex::traits::AreAllTrivial<Types...>(), Types...>;

	// #todo write proper option that is efficient, temporary hack
	template <typename T>
	using Opt = vex::union_impl::UnionImpl<std::is_trivial_v<T>, T>;
}
