#ifndef H_MODULES_CUTEHMI_u_STUPID_u_1_SRC_CUTEHMI_STUPID_PLUGIN_PLUGINNODEDATA_HPP
#define H_MODULES_CUTEHMI_u_STUPID_u_1_SRC_CUTEHMI_STUPID_PLUGIN_PLUGINNODEDATA_HPP

#include <cutehmi/xml/BackendPlugin.hpp>

#include <cutehmi/DataBlock.hpp>

namespace cutehmi {
namespace stupid {
namespace plugin {

class PluginNodeData:
	public DataBlock
{
	public:
		PluginNodeData(xml::IBackendPlugin * implementation);

		xml::BackendPlugin * xmlBackendPlugin() const;

	private:
		std::unique_ptr<xml::BackendPlugin> m_xmlBackendPlugin;
};

}
}
}

#endif

//(c)MP: Copyright © 2018, Michal Policht. All rights reserved.
//(c)MP: This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
