/*
 * Copyright (C) 2016  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_RESOURCE_ASSIGNMENT_H_
#define BBQUE_RESOURCE_ASSIGNMENT_H_

#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <string>

#include "bbque/res/bitset.h"
#include "bbque/res/resources.h"
#include "bbque/utils/utility.h"

namespace bbque
{
namespace res
{

// Forward declaration
class ResourceAssignment;
class ResourcePath;

/** Shared pointer to Usage object */
typedef std::shared_ptr<ResourceAssignment> ResourceAssignmentPtr_t;
/** Map of Usage descriptors. Key: resource path */
typedef std::map<ResourcePathPtr_t, ResourceAssignmentPtr_t, CompareSP<ResourcePath> > ResourceAssignmentMap_t;
/** Constant pointer to the map of Usage descriptors */
typedef std::shared_ptr<ResourceAssignmentMap_t> ResourceAssignmentMapPtr_t;


/**
 * @class ResourceAssignment
 * @brief How the resource requests are bound into assignments
 *
 * An application working modes defines a set of this resource requests
 * (usages).
 *
 * A resource usage descriptor embeds a couple of information:
 * The first is obviously the value of the usage, the amount of resource
 * requested. The second is a list containing all the descriptors (shared
 * pointer) of the resources which to this usage refers. We expect that such
 * list is filled by a method of ResourceAccounter, after that the
 * Scheduler/Optimizer has solved the resource binding.
 *
 * The "resource binding" can be explained as follows: if a Working Mode
 * includes resource requests like "tile.cluster2.pe = 4", the
 * scheduler/optimizer must define a binding between that "cluster2" and the
 * "real" cluster on the platform, to which bind the usage of 4 processing
 * elements. Thus, after that, the list "resources", will contain the
 * descriptors of the resources "pe (processing elements)" in the cluster
 * assigned by the scheduler/optimizer module.
 */
class ResourceAssignment {

friend class bbque::ResourceAccounter;

public:

	/**
	 * @enum ExitCode_t
	 */
	enum ExitCode_t {
	        /** Success */
	        RU_OK = 0,
	        /** Application pointer is null */
	        RU_ERR_NULL_POINTER,
	        /** Application pointer mismatch */
	        RU_ERR_APP_MISMATCH,
	        /** Resource state view token mismatch */
	        RU_ERR_VIEW_MISMATCH
	};

	/**
	 * @enum class Policy
	 *
	 * The usage must be bind to a set of physical resources. This can be done
	 * by the resource allocation policy through a per-resource fine-grained
	 * binding, or can be left to the Resource Accounter, specifying a
	 * predefined "filling" policy. The policy defines how the amount of
	 * resource required must be spread over the set of physical resources
	 * that the Usage object is referencing.
	 */
	enum class Policy
	{
	        /**
	         * Usage should be distributed over the resource list in a sequential
	         * manner
	         */
	        SEQUENTIAL,
	        /**
	         * Usage should be evenly distributed over all the resources in the
	         * list
	         */
	        BALANCED
	};

	/**
	 * @brief Constructor

	 * @param amount The amount of resource usage
	 * @param policy The filling policy
	 */
	ResourceAssignment(uint64_t amount, Policy policy = Policy::SEQUENTIAL);

	/**
	 * @brief Destructor
	 */
	~ResourceAssignment();

	/**
	 * @brief The amount of resource required/assigned to the Application/EXC
	 *
	 * @return The amount of resource
	 */
	uint64_t GetAmount();

	/**
	 * @brief Set the amount of resource
	 */
	void SetAmount(uint64_t value);

	/**
	 * @brief Get the entire list of resources
	 *
	 * @return A reference to the resources list
	 */
	ResourcePtrList_t & GetResourcesList();

	/**
	 * @brief Set the list of resources
	 *
	 * Commonly a Usage object specifies the request of a specific
	 * type of resource, which can be bound on a set of platform resources
	 * (i.e. "sys0,cpu2.pe" -> "...cpu2.pe{0|1|2|...}".
	 * The resources list includes the pointers to all the resource descriptors
	 * that can satisfy the request. The method initialises the iterators
	 * pointing to the set of resources effectively granted to the
	 * Application/EXC.
	 *
	 * @param r_list The list of resource descriptor for binding
	 */
	void SetResourcesList(ResourcePtrList_t & r_list);

	/**
	 * @brief Set the a filtered list of resources
	 *
	 * Commonly a Usage object specifies the request of a specific
	 * type of resource, which can be bound on a set of platform resources
	 * (i.e. "sys0,cpu2.pe" -> "...cpu2.pe{0|1|2|...}".
	 *
	 * The resources list, in this case, includes the pointers to a subset of
	 * the resource descriptors that can satisfy the request. This subset
	 * is built on the bases of two parameters that allow the definition of a
	 * filter criteria.
	 *
	 * Considering the example above, for the path "sys0,.cpu2.pe" i can
	 * specify a filtered list of processing elements, such that it includes
	 * only PE2 and PE3: "sys0.cpu2.pe" -> "sys0cpu2.pe{2|3}"
	 *
	 * @param r_list The list of resource descriptor for binding
	 * @param filter_rtype The type of resource on which apply the
	 * filter
	 * @param filter_mask A bitmask where in the set bits represents the
	 * resource ID to include in the list
	 */
	void SetResourcesList(
	        ResourcePtrList_t & r_list,
	        ResourceType filter_rtype,
	        ResourceBitset & filter_mask);

	/**
	 * @brief Check of the resource binding list is empty
	 *
	 * @return true if the list is empty, false otherwise.
	 */
	inline bool EmptyResourcesList() const {
		return resources.empty();
	}


	/**
	 * @brief Set the resources list filling policy
	 */
	inline void SetPolicy(Policy policy) {
		fill_policy = policy;
	}

	/**
	 * @brief Get the resources list filling policy
	 *
	 * @return The policy set
	 */
	inline Policy GetPolicy() const {
		return fill_policy;
	}


	/**
	 * @brief Get the first resource from the binding list
	 *
	 * @param it The list iterator. This is set and hence must be used for
	 * iteration purposes.
	 *
	 * @return The pointer (shared) to the resource descriptor providing a
	 * first quota (or the whole of it) of the resource usage required.
	 */
	ResourcePtr_t GetFirstResource(ResourcePtrListIterator_t & it);

	/**
	 * @brief Get the last resource from the binding list
	 *
	 * @param it The list iterator returned by GetFirstResource() or a
	 * previous call to GetNextResource().
	 *
	 * @return The pointer (shared) to the resource descriptor providing a
	 * quota of the resource usage required.
	 */
	ResourcePtr_t GetNextResource(ResourcePtrListIterator_t & it);

	/**
	 * @brief Track the first resource, from the binding list, granted to the
	 * Application/EXC
	 *
	 * The binding list tracks the whole set of possible resources to which
	 * bind the usage request. This method set the first resource granted to
	 * the Application/EXC, to satisfy part of the usage request, or the whole
	 * of it.
	 *
	 * @param papp The Application/EXC requiring the resource
	 * @param first_it The list iterator of the resource binding to track
	 * @param status_view The token of the resource state view into which the
	 * assignment has been performed.
	 *
	 * @return RU_OK if success. RU_ERR_NULL_POINTER if the pointer to the
	 * application is null.
	 */
	ExitCode_t TrackFirstResource(AppSPtr_t const & papp,
	                              ResourcePtrListIterator_t & first_it, RViewToken_t status_view);

	/**
	 * @brief Track the last resource, from the binding list, granted to the
	 * Application/EXC
	 *
	 * The binding list tracks the whole set of possible resources to which
	 * bind the usage request. This method set the last resource granted to
	 * the Application/EXC, to satisfy the last quota of the usage request.
	 *
	 * @param papp The Application/EXC requiring the resource
	 * @param last_it The list iterator of the resource binding to track
	 * @param status_view The token of the resource state view into which the
	 * assignment has been performed.
	 *
	 * @return RU_OK if success.
	 * RU_ERR_NULL_POINTER if the pointer to the application is null.
	 * RU_ERR_APP_MISMATCH if the application specified differs from the one
	 * specified in TrackFirstResource().
	 * RU_ERR_VIEW_MISMATCH if the state view token does not match the one set
	 * in TrackFirstResource().
	 */
	ExitCode_t TrackLastResource(AppSPtr_t const & papp,
	                             ResourcePtrListIterator_t & last_it, RViewToken_t status_view);

private:

	/** Usage amount request */
	uint64_t amount = 0;

	/** List of resource descriptors which to the resource usage is bound */
	ResourcePtrList_t resources;

	/**
	 * The resources list filling policy, i.e., how the resource amount
	 * should be distributed over the resources list
	 */
	Policy fill_policy;

	/** List iterator pointing to the first resource used by the App/EXC */
	ResourcePtrListIterator_t first_resource_it;

	/** List iterator pointing to the last resource used by the App/EXC */
	ResourcePtrListIterator_t last_resource_it;

	/** The application/EXC owning this resource usage */
	AppSPtr_t owner_app;

	/** The token referencing the state view of the resource usage */
	RViewToken_t status_view = 0;

};

} // namespace res

} // namespace bbque

#endif // BBQUE_RESOURCE_USAGE_H
