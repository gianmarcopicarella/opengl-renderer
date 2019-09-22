#pragma once
#define MAX_ANIMATIONS 10
#include <gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <unordered_map>
#include <set>

typedef struct PositionKeyFrame {
	float time;
	glm::vec3 value;
} PositionKeyFrame;

typedef struct RotationKeyFrame {
	float time;
	glm::quat value;
} RotationKeyFrame;

typedef struct ScalingKeyFrame {
	float time;
	glm::vec3 value;
} ScalingKeyFrame;

typedef struct AnimChannel {
	PositionKeyFrame* position_keyframe;
	RotationKeyFrame* rotation_keyframe;
	ScalingKeyFrame* scaling_keyframe;

	int position_keycount;
	int rotation_keycount;
	int scaling_keycount;
} AnimChannel;

typedef struct Animation {
	std::unordered_map<std::string, AnimChannel> joints_channel;
	float ticks_per_second, length;
} Animation;

typedef struct SkeletalNode {
	std::string name;
	glm::mat4 final_transform;
	glm::mat4 m_transform;
	glm::mat4 offset_transform = glm::mat4(1.0f);
	int index_transform = -1;

	SkeletalNode** children;
	int children_count;

} SkeletalNode;

static inline glm::vec3 vec3_cast(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
static inline glm::vec2 vec2_cast(const aiVector3D& v) { return glm::vec2(v.x, v.y); }
static inline glm::quat quat_cast(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
static inline glm::mat4 mat4_cast(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }
static inline glm::mat4 mat4_cast(const aiMatrix3x3& m) { return glm::transpose(glm::make_mat3(&m.a1)); }

class AnimationController {
public:
	AnimationController(const aiScene* scene);
	~AnimationController();
	void pause();
	void resume();
	void set_speed(float factor);
	void set_animation(int animation_index);
	void update(float delta_time);

	void send_transform_to_shader(GLuint location);

	SkeletalNode* get_node_by_name(std::string name);
	int get_joint_count();
protected:
private:
	void read_node_heirarchy(SkeletalNode* node, glm::mat4* parent_transfor, float* animation_time);
	void parse_animation(const aiScene* scene, int animation_index);
	SkeletalNode* parse_skeleton(const aiNode* node, std::set<std::string>* bones);
	std::set<std::string> get_joints_set(const aiScene* scene);

	/* test funcs */
	void read_node_hierarchy_transition(SkeletalNode* node, glm::mat4* parent_transform, float* animation_time1, float* animation_time2, float* factor);

	float factor_speed, time;
	int last_animation, curr_animation;
	
	int animation_counter;
	Animation animations[MAX_ANIMATIONS];
	
	bool transition;
	float blending_time, transition_time;
	
	SkeletalNode* root;
	glm::mat4 inverse_bind_transform;

	std::unordered_map<std::string, SkeletalNode*> bones;
	glm::mat4* final_transforms;
};
	