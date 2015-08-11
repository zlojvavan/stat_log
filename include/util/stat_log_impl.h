#pragma once
#include "fusion_includes.h"
//using namespace boost::fusion;

#define NAME static constexpr const char* name
namespace rc
{
namespace stat_log
{

template <typename T, bool IsOperational>
struct stat_traits;

namespace detail
{
   using namespace boost;
   using namespace boost::fusion;

   template <typename U> struct AlwaysVoid {
      typedef void type;
   };
   template <typename Tag, typename MatchingTags, bool IsOperational>
   struct GenericStat
   {
      void setShmPtr(void* shm_ptr)
      {
         theProxy.setShmPtr(shm_ptr);
      }
      using SerialType = typename stat_traits<Tag, IsOperational>::SerialType;
      using Proxy = typename stat_traits<Tag, IsOperational>::ProxyType;
      Proxy theProxy;
      using matching_tags = MatchingTags;

      auto& getValue()
      {
         return this->theProxy.getValue();
      }
   };

   template <typename Tag, typename MatchingTags>
      struct GenericOpStat : GenericStat<Tag, MatchingTags, true>
   {
      template <typename... Args>
         void writeVal(Args... args)
         {
            this->theProxy.write(args...);
         }

      using BaseClass = GenericStat<Tag, MatchingTags, true>;
      using SerialType = typename BaseClass::SerialType;
   };


   template <typename Tag, typename MatchingTags>
      struct GenericControlStat : GenericStat<Tag, MatchingTags, false>
   {
      using BaseClass = GenericStat<Tag, MatchingTags, false>;
      using SerialType = typename BaseClass::SerialType;
   };

   template <typename T>
      struct is_parent
      {
         //T is of type "struct TagString"
         //Then T::child_list should be either another sequence (if parent) or int
         static const bool value = mpl::is_sequence<typename T::child_list>::value;
      };

   template <typename GlobalTagVec, typename ParentVec,
             typename ThisStat ,typename IsOpType
             >
      struct stat_inserter
      {
         //ThisStat is of type "struct TagString"
         using ThisTag = typename ThisStat::tag;
         //Add the entire lineage of parents to the matching tags for
         // this statistic
         using matching_tags =
            typename mpl::push_back<ParentVec,ThisTag>::type;
         using this_stat = typename std::conditional_t<
               IsOpType::value,
               GenericOpStat<ThisTag, matching_tags>,
               GenericControlStat<ThisTag, matching_tags>
               >;
         //Finally add this Repr to the global tag vec
         using type = typename mpl::push_back<GlobalTagVec, this_stat>::type;
      };

   template <typename GlobalTagVec, typename ParentVec,
             typename TagHierarchy , typename IsOpType
             >

      struct stat_creator_helper
      {

         using ThisLineage = typename mpl::push_back<ParentVec,
               typename TagHierarchy::tag>::type;
         using ChildTagHierarchy = typename TagHierarchy::child_list;

         // _1 == GlobalTagVec
         // _2 == the iterator to the child TagString
         using type = typename mpl::fold<
               ChildTagHierarchy,
               GlobalTagVec,
               mpl::eval_if<
                  is_parent<mpl::_2>,
                  stat_creator_helper<
                     mpl::_1, ThisLineage, mpl::_2, IsOpType
                  >,
                  stat_inserter<mpl::_1, ThisLineage, mpl::_2, IsOpType
                  >
               >
            >::type;
      };

   template <typename TagHierarchy, bool IsOperational>
      struct stat_creator
      {
         // using IsOpType = typename std::integral_constant<bool, true>;
         using type = typename stat_creator_helper<
            mpl::vector<>, //Global Tag/Stat Vector
            mpl::vector<>, //Parent vector
            TagHierarchy,
            //Need to wrap the boolean to be nice to the MPL algorithms
            typename std::integral_constant<bool, IsOperational>
            >::type;
      };

   template <typename TagVec, typename Tag>
      struct contains_tag
      {
         using type = typename mpl::contains<TagVec, Tag>::type;
      };

   template <typename Tag>
      struct matches_tag
      {
         template <typename Repr>
            struct apply
            : contains_tag<typename Repr::matching_tags, Tag>::type
            {};
      };

   template <typename T, typename P, typename C = void>
      struct TagString
      {
         NAME = T::name;
         using tag = T;
         using parent = P;
         using child_list = C;
      };

   template <typename T, typename Parent, typename Dummy = void>
      struct GenTagHierarchy
      {
         using type = TagString<T, Parent>;
      };

   template <typename T, typename Parent>
      struct GenTagHierarchy<T, Parent, typename AlwaysVoid<typename T::ChildTypes>::type>
      {
         using ChildTypes = typename T::ChildTypes;
         using child_type_vec = typename boost::mpl::transform<
            ChildTypes, GenTagHierarchy<boost::mpl::_, T>>::type;
         using type = TagString<T, Parent, child_type_vec>;
      };

   template <typename UserStatH, bool IsOperational>
      struct LogStatBase
   {
      struct TopName
      {
         NAME = "";
      };
      using Top = detail::TagString<TopName, void>;
      public:
         using TagHierarchy = typename detail::GenTagHierarchy<UserStatH, Top>::type;
         using TheStats = typename boost::fusion::result_of::as_vector<
            typename detail::stat_creator<TagHierarchy, IsOperational>::type>::type;

         void assignShmPtr(char* shm_ptr)
         {
            using namespace boost::fusion;
            for_each(theStats, [&shm_ptr](auto& stat)
                  {
                  using StatType = std::remove_reference_t<decltype(stat)>;
                  stat.setShmPtr(shm_ptr);
                  std::cout << "assignSmPtr to " << TypeId<StatType>{}
                  << std::hex << (long int)shm_ptr << std::endl;
                  shm_ptr += sizeof(typename StatType::SerialType);
                  });

         }
         TheStats theStats;
   };
}

}
}