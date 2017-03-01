#ifndef PRESET_H
#define PRESET_H

#include <string>

namespace VSTHost {
class Preset {
public:
	virtual ~Preset() {}
	virtual bool Load() = 0;
	virtual bool Load(const std::string& path) = 0;
	virtual bool Save() = 0;
	virtual bool Save(const std::string& path) = 0;
protected:
	virtual void SetState() = 0;
	virtual void GetState() = 0;
	std::string preset_file_path;
};
} // namespace

#endif