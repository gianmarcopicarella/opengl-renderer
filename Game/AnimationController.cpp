#include "AnimationController.h"



unsigned int FindPosition(float AnimationTime, AnimChannel anim_ch)
{
	for (unsigned int i = 0; i < anim_ch.position_keycount - 1; i++) {
		if (AnimationTime < (float)anim_ch.position_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
unsigned int FindRotation(float AnimationTime, AnimChannel anim_ch)
{

	assert(anim_ch.rotation_keycount > 0);

	for (unsigned int i = 0; i < anim_ch.rotation_keycount - 1; i++) {
		if (AnimationTime < (float)anim_ch.rotation_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
unsigned int FindScaling(float AnimationTime, AnimChannel anim_ch)
{

	assert(anim_ch.scaling_keycount > 0);

	for (unsigned int i = 0; i < anim_ch.scaling_keycount - 1; i++) {
		if (AnimationTime < (float)anim_ch.scaling_keyframe[i + 1].time) {
			return i;
		}
	}

	assert(0);

	return 0;
}
glm::vec3 CalcInterpolatedPosition(float AnimationTime, AnimChannel anim_ch)
{
	if (anim_ch.position_keycount == 1) {
		return anim_ch.position_keyframe[0].value;
	}

	unsigned int ind = FindPosition(AnimationTime, anim_ch);
	unsigned int next_ind = ind + 1;
	assert(next_ind < anim_ch.position_keycount);
	float DeltaTime = (float)(anim_ch.position_keyframe[next_ind].time - anim_ch.position_keyframe[ind].time);
	float Factor = glm::min(glm::max((AnimationTime - (float)anim_ch.scaling_keyframe[ind].time) / DeltaTime, 0.f), 1.f);
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::mix(anim_ch.position_keyframe[ind].value, anim_ch.position_keyframe[next_ind].value, Factor);
}
glm::quat CalcInterpolatedRotation(float AnimationTime, AnimChannel anim_ch)
{
	// we need at least two values to interpolate...
	if (anim_ch.rotation_keycount == 1) {
		return anim_ch.rotation_keyframe[0].value;
	}

	unsigned int ind = FindRotation(AnimationTime, anim_ch);
	unsigned int next_ind = ind + 1;
	assert(next_ind < anim_ch.rotation_keycount);
	float DeltaTime = (float)(anim_ch.rotation_keyframe[next_ind].time - anim_ch.rotation_keyframe[ind].time);
	float Factor = glm::min(glm::max((AnimationTime - (float)anim_ch.scaling_keyframe[ind].time) / DeltaTime, 0.f), 1.f);
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::slerp(anim_ch.rotation_keyframe[ind].value, anim_ch.rotation_keyframe[next_ind].value, Factor);
}
glm::vec3 CalcInterpolatedScaling(float AnimationTime, AnimChannel anim_ch)
{
	if (anim_ch.scaling_keycount == 1) {
		return anim_ch.scaling_keyframe[0].value;
	}

	unsigned int ind = FindScaling(AnimationTime, anim_ch);
	unsigned int next_ind = (ind + 1);
	assert(next_ind < anim_ch.scaling_keycount);
	float DeltaTime = (float)(anim_ch.scaling_keyframe[next_ind].time - anim_ch.scaling_keyframe[ind].time);
	float Factor = glm::min(glm::max((AnimationTime - (float)anim_ch.scaling_keyframe[ind].time) / DeltaTime, 0.f), 1.f);
	assert(Factor >= 0.0f && Factor <= 1.0f);
	return glm::mix(anim_ch.scaling_keyframe[ind].value, anim_ch.scaling_keyframe[next_ind].value, Factor);
}

void free_skeleton(SkeletalNode* node, int animation_count) {
	for (int i = 0; i < node->children_count; i++) {
		free_skeleton(node->children[i], animation_count);
	}

	free(node->children);
	delete node;
}

void AnimationController::read_node_hierarchy_transition(SkeletalNode* node, glm::mat4* parent_transform, float* animation_time1, float* animation_time2, float * factor) {

	glm::mat4 node_transformation = node->m_transform;

	if (node->index_transform != -1) {

		AnimChannel anim_ch1 = this->animations[this->last_animation].joints_channel.find(node->name)->second;
		AnimChannel anim_ch2 = this->animations[this->curr_animation].joints_channel.find(node->name)->second;

		// Interpolate scaling and generate scaling transformation matrix
		glm::mat4 scaling_m = glm::scale(glm::mat4(1.0f), glm::mix(CalcInterpolatedScaling(*animation_time1, anim_ch1), CalcInterpolatedScaling(*animation_time2, anim_ch2), *factor));

		// Interpolate rotation and generate rotation transformation matrix
		glm::mat4 rotation_m = glm::toMat4(glm::slerp(CalcInterpolatedRotation(*animation_time1, anim_ch1), CalcInterpolatedRotation(*animation_time2, anim_ch2), *factor));

		// Interpolate translation and generate translation transformation matrix
		glm::mat4 translation_m = glm::translate(glm::mat4(1.0f), glm::mix(CalcInterpolatedPosition(*animation_time1, anim_ch1), CalcInterpolatedPosition(*animation_time2, anim_ch2), *factor));

		// Combine the above transformations
		node_transformation = translation_m * scaling_m * rotation_m;
	}

	// Combine with node Transformation with Parent Transformation
	glm::mat4 global_transformation = *parent_transform * node_transformation;

	if (node->index_transform != -1) {
		node->final_transform = this->inverse_bind_transform * global_transformation * node->offset_transform;
		this->final_transforms[node->index_transform] = node->final_transform;
	}

	for (unsigned int i = 0; i < node->children_count; i++) {
		read_node_hierarchy_transition(node->children[i], &global_transformation, animation_time1, animation_time2, factor);
	}
}

AnimationController::AnimationController(const aiScene* scene)
{
	this->time = 0.f;
	this->factor_speed = 1.0f;
	this->curr_animation = -1;
	this->last_animation = -1;
	this->animation_counter = 0;
	this->transition = false;
	this->blending_time = 0.f;
	this->transition_time = 2.f;

	std::set<std::string> joints = this->get_joints_set(scene);
	this->root = this->parse_skeleton(scene->mRootNode, &joints);
	this->final_transforms = (glm::mat4*)calloc(this->bones.size(), sizeof(glm::mat4));
	this->inverse_bind_transform = glm::inverse(mat4_cast(scene->mRootNode->mTransformation));
	for (int i = 0; i < glm::min((int)scene->mNumAnimations, MAX_ANIMATIONS); i++) {
		this->parse_animation(scene, i);
	}
}

AnimationController::~AnimationController()
{
	for (int i = 0; i < this->animation_counter; i++) {
		for (auto& x : this->animations[i].joints_channel) {
			free(x.second.position_keyframe);
			free(x.second.rotation_keyframe);
			free(x.second.scaling_keyframe);
		}
		this->animations[i].joints_channel.clear();
	}
	free_skeleton(this->root, this->animation_counter);
	free(this->final_transforms);
	this->bones.clear();

}

void AnimationController::pause()
{
	this->factor_speed = 0.0f;
}
void AnimationController::resume()
{
	this->factor_speed = 1.f;
}

void AnimationController::set_speed(float factor)
{
	this->factor_speed = factor;
}
void AnimationController::set_animation(int animation_index)
{
	assert(animation_index > -1 && animation_index < this->animation_counter);
	this->transition = this->curr_animation != -1;
	this->last_animation = this->curr_animation;
	this->curr_animation = animation_index;

	/* debug
	for (auto &x : this->bones) {
		printf("%s\n", x.first.c_str());
	}
	printf("bone count = %d\n", this->get_joint_count());
	*/
}
void AnimationController::update(float delta_time)
{
	glm::mat4 root_parent = glm::rotate(glm::radians(-90.f), glm::vec3(1.0f, 0, 0));

	// update main animation time
	this->time += delta_time * this->factor_speed;
		
	if (this->transition) {
		this->blending_time += delta_time * this->factor_speed;
		
		float time_in_ticks = this->time * this->animations[this->last_animation].ticks_per_second;
		float last_anim_time = fmod(time_in_ticks, this->animations[this->last_animation].length);

		time_in_ticks = this->blending_time * this->animations[this->curr_animation].ticks_per_second;
		float curr_anim_time = fmod(time_in_ticks, this->animations[this->curr_animation].length);
		
		float blending_factor = this->blending_time / this->transition_time;
	
		read_node_hierarchy_transition(this->root, &root_parent, &last_anim_time, &curr_anim_time, &blending_factor);
		if (blending_factor > 0.99f) {
			this->transition = false;
			this->time = this->blending_time;
			this->blending_time = 0;
		}
	}
	else {
		float time_in_ticks = this->time * this->animations[this->curr_animation].ticks_per_second;
		float animation_time = fmod(time_in_ticks, this->animations[this->curr_animation].length);
		this->read_node_heirarchy(this->root, &root_parent, &animation_time);
	}
}
void AnimationController::send_transform_to_shader(GLuint location)
{
	glUniformMatrix4fv(location,
		(int)this->bones.size(), GL_FALSE, glm::value_ptr(this->final_transforms[0]));
}
SkeletalNode* AnimationController::get_node_by_name(std::string name)
{
	if (this->bones.find(name) == this->bones.end()) return nullptr;
	return this->bones.find(name)->second;
}
int AnimationController::get_joint_count()
{
	return this->bones.size();
}
void AnimationController::read_node_heirarchy(SkeletalNode* node, glm::mat4* parent_transform, float * animation_time)
{
	glm::mat4 node_transformation = node->m_transform;

	if (node->index_transform != -1) {

		AnimChannel anim_ch = this->animations[this->curr_animation].joints_channel.find(node->name)->second;

		// Interpolate scaling and generate scaling transformation matrix
		glm::mat4 scaling_m = glm::scale(glm::mat4(1.0f), CalcInterpolatedScaling(*animation_time, anim_ch));

		// Interpolate rotation and generate rotation transformation matrix
		glm::mat4 rotation_m = glm::toMat4(CalcInterpolatedRotation(*animation_time, anim_ch));

		// Interpolate translation and generate translation transformation matrix
		glm::mat4 translation_m = glm::translate(glm::mat4(1.0f), CalcInterpolatedPosition(*animation_time, anim_ch));

		// Combine the above transformations
		node_transformation = translation_m * scaling_m * rotation_m;
	}

	// Combine with node Transformation with Parent Transformation
	glm::mat4 global_transformation = *parent_transform * node_transformation;

	if (node->index_transform != -1) {
		node->final_transform = this->inverse_bind_transform * global_transformation * node->offset_transform;
		this->final_transforms[node->index_transform] = node->final_transform;
	}

	for (unsigned int i = 0; i < node->children_count; i++) {
		read_node_heirarchy(node->children[i], &global_transformation, animation_time);
	}
}
void AnimationController::parse_animation(const aiScene* scene, int animation_index)
{
	Animation anim = {};
	anim.ticks_per_second = scene->mAnimations[animation_index]->mTicksPerSecond != 0 ?
		scene->mAnimations[animation_index]->mTicksPerSecond : 25.f;
	anim.length = scene->mAnimations[animation_index]->mDuration;

	for (unsigned int i = 0; i < scene->mAnimations[animation_index]->mNumChannels; i++) {
		aiNodeAnim* node_anim_data = scene->mAnimations[animation_index]->mChannels[i];

		AnimChannel anim_ch = {};

		anim_ch.position_keycount = node_anim_data->mNumPositionKeys;
		anim_ch.rotation_keycount = node_anim_data->mNumRotationKeys;
		anim_ch.scaling_keycount = node_anim_data->mNumScalingKeys;

		anim_ch.position_keyframe = (PositionKeyFrame*)calloc(node_anim_data->mNumPositionKeys, sizeof(PositionKeyFrame));

		for (int i = 0; i < node_anim_data->mNumPositionKeys; i++) {
			anim_ch.position_keyframe[i].time = node_anim_data->mPositionKeys[i].mTime;
			anim_ch.position_keyframe[i].value = vec3_cast(node_anim_data->mPositionKeys[i].mValue);
		}

		anim_ch.rotation_keyframe = (RotationKeyFrame*)calloc(node_anim_data->mNumRotationKeys, sizeof(RotationKeyFrame));

		for (int i = 0; i < node_anim_data->mNumRotationKeys; i++) {
			anim_ch.rotation_keyframe[i].time = node_anim_data->mRotationKeys[i].mTime;
			anim_ch.rotation_keyframe[i].value = quat_cast(node_anim_data->mRotationKeys[i].mValue);
		}

		anim_ch.scaling_keyframe = (ScalingKeyFrame*)calloc(node_anim_data->mNumScalingKeys, sizeof(ScalingKeyFrame));

		for (int i = 0; i < node_anim_data->mNumScalingKeys; i++) {
			anim_ch.scaling_keyframe[i].time = node_anim_data->mScalingKeys[i].mTime;
			anim_ch.scaling_keyframe[i].value = vec3_cast(node_anim_data->mScalingKeys[i].mValue);
		}

		anim.joints_channel.insert(std::pair<std::string, AnimChannel>(std::string(node_anim_data->mNodeName.C_Str()), anim_ch));
	}

	this->animations[this->animation_counter++] = anim;
}

SkeletalNode* AnimationController::parse_skeleton(const aiNode* node, std::set<std::string>* bones)
{
	SkeletalNode* my_node = new SkeletalNode();
	my_node->name = std::string(node->mName.data);
	my_node->m_transform = mat4_cast(node->mTransformation);

	if (bones->find(my_node->name) != bones->end()) {
		my_node->index_transform = this->bones.size();
		this->bones.insert(std::pair<std::string, SkeletalNode*>(my_node->name, my_node));
	}

	my_node->children_count = node->mNumChildren;
	my_node->children = (SkeletalNode * *)calloc(node->mNumChildren, sizeof(SkeletalNode*));

	for (int i = 0; i < node->mNumChildren; i++) {
		my_node->children[i] = parse_skeleton(node->mChildren[i], bones);
	}

	return my_node;
}

std::set<std::string> AnimationController::get_joints_set(const aiScene* scene)
{
	std::set<std::string> bones = {};
	for (unsigned int k = 0; k < scene->mNumAnimations; k++) {
		for (unsigned int i = 0; i < scene->mAnimations[k]->mNumChannels; i++) {
			aiNodeAnim* pNodeAnim = scene->mAnimations[k]->mChannels[i];
			std::string bone_name(pNodeAnim->mNodeName.data);
			if (bones.find(bone_name) == bones.end()) {
				bones.insert(bone_name);
			}
		}
	}
	return bones;
}
