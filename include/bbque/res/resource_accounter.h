/**
 *       @file  resource_accounter.h
 *      @brief  Resource accounter component for Barbeque RTRM
 *
 * This defines the component for managing the resource accounting.
 *
 * Each resource of system/platform should be properly registered in the
 * Resource accounter. It keeps track of the information upon availability,
 * total amount and used resources.
 * The information above are updated through proper methods which must be
 * called when an application working mode has triggered.
 *
 *     @author  Giuseppe Massari (jumanix), joe.massanga@gmail.com
 *
 *   @internal
 *     Created  04/04/2011
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2011, Giuseppe Massari
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * ============================================================================
 */

#ifndef BBQUE_RESOURCE_ACCOUNTER_H_
#define BBQUE_RESOURCE_ACCOUNTER_H_

#include "bbque/object.h"
#include "bbque/res/resource_accounter_conf.h"
#include "bbque/res/resource_tree.h"

#define RESOURCE_ACCOUNTER_NAMESPACE "res_acc."

namespace bbque { namespace res {

// Forward declarations
struct Resource;
struct ResourceUsage;
struct ResourceState;

typedef std::shared_ptr<Resource> ResourcePtr_t;
typedef std::shared_ptr<ResourceState> ResourceStatePtr_t;
typedef std::shared_ptr<ResourceUsage> UsagePtr_t;
typedef std::map<std::string, UsagePtr_t> UsagesMap_t;

/**
 * @class ResourceAccounter
 * @brief This component is used by the RTRM to keep track of the resources in
 * the system, their availability and usages information.
 *
 * It manages their status updating and information about which applications
 * are using a given resource.
 */
class ResourceAccounter: public ResourceAccounterConfIF, public Object {

public:

	/**
	 * @brief Return the instance of the ResourceAccounter
	 */
	static ResourceAccounter *GetInstance();

	/**
	 * @brief Destructor
	 */
	~ResourceAccounter();

	/**
	 * @see ResourceAccounterStatusIF
	 */
	inline uint64_t Available(std::string const & path) const {
		return stateInformation(path, RA_AVAILAB);
	}

	/**
	 * @see ResourceAccounterStatusIF
	 */
	inline uint64_t Total(std::string const & path) const {
		return stateInformation( path, RA_TOTAL);
	}

	/**
	 * @see ResourceAccounterStatusIF
	 */
	inline uint64_t Used(std::string const & path) const {
		return stateInformation(path, RA_USED);
	}

	/**
	 * @see ResourceAccounterStatusIF
	 */
	inline ResourcePtr_t GetResource(std::string const & path) {
		return resources.find(path);
	}

	/**
	 * @see ResourceAccounterStatusIF
	 */
	inline bool ExistResource(std::string const & path) {
		std::string templ_path = pathTemplate(path);
		return resources.match_path(templ_path);
	}

	/**
	 * @see ResourceAccounterConfIF
	 */
	inline void SwitchUsage(app::Application const * app) {
		changeUsages(app, RA_SWITCH);
	}

	/**
	 * @see ResourceAccounterConfIF
	 */
	inline void Release(app::Application const * _app) {
		changeUsages(_app, RA_RELEASE);
	}

	/**
	 * @see ResourceAccounterConfIF
	 */
	void RegisterResource(std::string const & path, std::string const & type,
			std::string const & units, uint32_t amount);

	/**
	 * @brief Print the resource hierarchy in a tree-like form
	 */
	inline void TreeView() {
		resources.print_tree();
	}

private:

	/**
	 * @enum This is used for selecting the state attribute to return from
	 * <tt>stateInformation()</tt>
	 */
	enum AttributeSelector_t {
		/** Amount of resource available */
		RA_AVAILAB = 0,
		/** Amount of resource used */
		RA_USED,
		/** Total amount of resource */
		RA_TOTAL
	};

	/**
	 * @enum This is used for action selection in <tt>_ChangeUsages()</tt>
	 */
	enum UsageAction_t {
		/**
		 * Switch resource usages of the application to the one of the next
		 * working mode
		 */
		RA_SWITCH,
		/** Release all the resource used by the application */
		RA_RELEASE
	};

	/**
	 * Default constructor
	 */
	ResourceAccounter();

	/**
	 * @brief Given a specific resource path (i.e.
	 * "arch.clusters.cluster2.pe1") return the template format:
	 * "arch.clusters.cluster.pe".
	 *
	 * This is useful for checking resource existance without refer to a
	 * specific resource object (with its id-based path). This way let us to
	 *
	 * @param path Resource complete path (ID-based)
	 * @return A template path (without resource IDs)
	 */
	std::string const pathTemplate(std::string const & path);

	/**
	 * @brief Return a state parameter (availability, resources used, total
	 * amount) for the resource.
	 * @param path Resource path
	 * @param sel Resource attribute request (@see AttributeSelector_t)
	 * @return The value of the attribute request
	 */
	uint64_t stateInformation(std::string const & path,
			AttributeSelector_t	sel) const;

	/**
	 *  @brief Change the set of resources usages of the given application.
	 *
	 *  This occours whenever an application has switched schedule status
	 *  and/or working mode.
	 *
	 *  @param app The descriptor pointer to the application changing the
	 *  resource usages
	 *  @param action What kind of change? (@see UsageAction_t)
	 */
	void changeUsages(app::Application const * app, UsageAction_t action);

	/** System logger instance */
	plugins::LoggerIF * logger;

	/** The tree of all the resources in the system.*/
	ResourceTree resources;

	/**
	 * The map of resource usages specified in the working modes of each
	 * application. The key is the application name through which lookup the
	 * list of current usages (relative to the current working mode)
	 */
	std::map<std::string, UsagesMap_t const *> usages;

};

}   // namespace res

}   // namespace bbque

#endif  // BBQUE_RESOURCE_ACCOUNTER_H_

