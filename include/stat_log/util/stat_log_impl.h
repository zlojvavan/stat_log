#pragma once
//TODO: generalize backend types
#include <stat_log/defs.h>
#include <stat_log/backends/shared_mem_backend.h>
#include <stat_log/loggers/logger_common.h>
#include <stat_log/stats/stats_common.h>

#include <boost/fusion/view.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/deref.hpp>
#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/for_each.hpp>

#include <boost/fusion/include/as_list.hpp>
#include <boost/fusion/include/list.hpp>

#include <boost/mpl/arg.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/contains.hpp>

#include <type_traits>
#include <string>

namespace stat_log
{

namespace detail
{
   using namespace boost;
   using namespace boost::fusion;


   //++++ LOGGER CONTAINERS ++++++++++
   template <typename TagNode>
   struct GenericOpLogger
   {
      using tag_node = TagNode;
      using tag = typename tag_node::tag;
      void setSharedPtr(void* ptr)
      {
         theProxy.setSharedPtr(ptr);
      }

      void doSerialize()
      {
      }

      using Proxy = LogOpProxy;
      Proxy theProxy;
      static constexpr bool IsParent = true;
   };

   template <typename TagNode>
   struct GenericControlLogger
   {
      using tag_node = TagNode;
      using tag = typename tag_node::tag;
      void setSharedPtr(void* ptr)
      {
         theProxy.setSharedPtr(ptr);
      }

      using Proxy = LogControlProxy;
      Proxy theProxy;
      static constexpr bool IsParent = true;
   };
   //+++++++++++++++++++++++++++++++++

   //++++ STATISTIC CONTAINERS ++++++++++
   template <typename TagNode>
   struct GenericOpStat
   {
      using tag_node = TagNode;
      using tag = typename tag_node::tag;
      static constexpr bool IsParent = false;

      template <typename... Args>
      void writeVal(Args... args)
      {
         theProxy.writeVal(args...);
      }

      void setSharedPtr(void* ptr)
      {
         theProxy.setSharedPtr(ptr);
      }
      using Proxy = OperationalStatProxy<typename stat_tag_to_type<tag>::type>;
      Proxy theProxy;

      void doSerialize()
      {
         theProxy.serialize();
      }
   };


   template <typename TagNode>
   struct GenericControlStat
   {
      using tag_node = TagNode;
      using tag = typename tag_node::tag;
      static constexpr bool IsParent = false;

      void setSharedPtr(void* ptr)
      {
         theProxy.setSharedPtr(ptr);
      }
      using Proxy = ControlStatProxy<typename stat_tag_to_type<tag>::type>;
      Proxy theProxy;
   };
   //+++++++++++++++++++++++++++++++++

   ///////////////

   template <typename T>
   struct is_parent
   {
      //T is of type "struct TagNode"
      //Then T::child_list should be either another sequence (if parent) or void
      using type = typename mpl::is_sequence<typename T::child_list>::type;
      static const bool value = mpl::is_sequence<typename T::child_list>::value;
   };

   template <typename GlobalTagVec, typename ParentVec,
             typename ThisStat ,typename IsOpType
             >
   struct stat_inserter
   {
      //ThisStat is of type "struct TagNode"
      using this_stat = typename std::conditional_t<
         IsOpType::value,
         GenericOpStat<ThisStat>,
         GenericControlStat<ThisStat>
      >;

      //Finally add this statistic to the global tag vec
      using type = typename mpl::push_front<GlobalTagVec, this_stat>::type;
   };

   template <typename GlobalTagVec, typename ParentVec,
             typename TagHierarchy , typename IsOpType
             >
   struct stat_creator_helper
   {

      using ThisLineage = typename mpl::push_front<ParentVec,
            typename TagHierarchy::tag>::type;
      using ChildTagHierarchy = typename TagHierarchy::child_list;
      using this_logger = typename std::conditional_t<
            IsOpType::value,
            GenericOpLogger<TagHierarchy>,
            GenericControlLogger<TagHierarchy>
         >;

      using UpdatedGlobalTagVec = typename mpl::push_front<
         GlobalTagVec, this_logger>::type;

      // _1 == UpdatedGlobalTagVec
      // _2 == the iterator to the child TagNode
      using type = typename mpl::fold<
            ChildTagHierarchy,
            UpdatedGlobalTagVec,
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
      using type = typename stat_creator_helper<
         mpl::list<>, //Global Tag/Stat list
         mpl::list<>, //Parent list
         TagHierarchy,
         //Need to wrap the boolean to be nice to the MPL algorithms
         typename std::integral_constant<bool, IsOperational>
         >::type;
   };

   //This template is used in conjunction with an MPL algorithm
   // with the same semantics as mpl::find_if.
   //BoolFunc is the "condition" metafunction.
   //StatTagFunc is a metafunction that transforms the given
   //   stat_tag into something the algorithm requires.
   //   For example the "Identity" metafunction would work here.
   //StatTagArgs is extra arguments to the BoolFunc
   template <template<typename...> class BoolFunc,
             template<typename...> class StatTagFunc,
             class... StatTagArgs>
   struct tag_node_query
   {
      template<typename T>
      struct apply
      {
         using stat_tag = typename T::tag;
         using type = std::integral_constant
            <
               bool,
               BoolFunc<
                  typename StatTagFunc<stat_tag>::type,
                  StatTagArgs...
               >::value
            >;
      };
   };

   template <typename Tag>  using matches_tag
      = tag_node_query<std::is_same, Identity, Tag>;

   template <typename T, typename P, int Depth, typename C = void>
   struct TagNode
   {
      SL_NAME = T::name;
      using tag = T;
      using parent = P;
      static const int depth = Depth;
      using child_list = C;
   };

   template <typename T, typename Parent, typename Depth, class Dummy = void>
   struct GenTagHierarchy
   {
      using type = TagNode<T, Parent, Depth::value>;
   };

   template <typename T, typename Parent, typename Depth>
   struct GenTagHierarchy<T, Parent, Depth,
      typename AlwaysVoid<typename T::ChildTypes>::type>
   {
      using ChildTypes = typename T::ChildTypes;
      using ChildDepth = std::integral_constant<int, Depth::value + 1>;
      using child_type_vec = typename boost::mpl::transform<
         ChildTypes, GenTagHierarchy<boost::mpl::_, T, ChildDepth>>::type;
      using type = TagNode<T, Parent, Depth::value, child_type_vec>;
   };

   template <typename UserStatDefs, bool IsOperational, typename Derived>
   struct LogStatBase
   {
      struct TopName
      {
         SL_NAME = "";
      };
      using TopNode = detail::TagNode<TopName, void, 0>;

      using TagHierarchy = typename detail::GenTagHierarchy<UserStatDefs, TopNode,
            std::integral_constant<int, 0> >::type;
      using TheStats = typename boost::fusion::result_of::as_list<
         typename detail::stat_creator<TagHierarchy, IsOperational>::type>::type;

      //Creates a single shared memory block that will store:
      //  -- The serialization for each stat.
      //  -- The control block for each stat (this will be
      //     used to signal per-stat behavior -- e.g., clear
      //     disable, etc.).
      //  -- The control block for each parent (this will be
      //     used to set the log level for both the normal logging
      //     AND the hex dumps).
      //1. Control Block
      //2. Stat Block
      void init(const std::string& shm_name)
      {
         size_t total_shm_size = 0;
         using namespace boost::fusion;
         for_each(theStats, [&total_shm_size](auto& stat)
         {
            using StatType = std::remove_reference_t<decltype(stat)>;
            total_shm_size += StatType::Proxy::getSharedSize();
         });
         shm_backend.setParams(shm_name, total_shm_size, IsOperational);
         auto shm_start = shm_backend.getMemoryPtr();
#if 0
         std::cout << std::dec <<  "SHM size = " << total_shm_size
            <<" , shm_start = " << std::hex << (long int)shm_start << std::endl;
#endif
         auto shm_ptr = shm_start;

         //Next, need to inform each node theStats about its location
         // in shared memory
         for_each(theStats, [&shm_ptr](auto& stat)
         {
            using StatType = std::remove_reference_t<decltype(stat)>;
            stat.setSharedPtr(shm_ptr);
            shm_ptr += StatType::Proxy::getSharedSize();
         });

         static_cast<Derived*>(this)->doInit();
      }

      void stop()
      {
         static_cast<Derived*>(this)->doStop();
      }

      TheStats theStats;
      shared_mem_backend shm_backend;
   };

   template<typename Tag, typename T>
   auto getStatHandleView(T& stats)
   {
      return boost::fusion::filter_view<T, detail::matches_tag<Tag>>(stats);
   }
}
}
