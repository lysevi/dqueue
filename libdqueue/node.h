#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/q.h>

namespace dqueue {
class Node {
public:
  struct Settings {};
  struct QueueDescription {
	  QueueSettings settings;
  };
  EXPORT Node(const Settings &settigns);
  EXPORT ~Node();
  EXPORT void createQueue(const QueueSettings&qsettings);
  EXPORT std::vector<QueueDescription> getDescription()const;
protected:
  struct Private;
  std::unique_ptr<Private> _impl;
};
} // namespace dqueue