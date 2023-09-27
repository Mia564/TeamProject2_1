#include "misc.h"
#include "skinned_mesh.h"
#include "shader.h"
#include "texture.h"
#include <sstream>
#include <fstream>
#include <functional>
#include <filesystem>
using namespace DirectX;

XMFLOAT4X4 to_xmfloat4x4(const FbxAMatrix& fbxamatrix);
XMFLOAT3 to_xmfloat3(const FbxDouble3& fbxdouble3);
XMFLOAT4 to_xmfloat4(const FbxDouble4& fbxdouble4);

struct BoneInfluence { // 骨の影響度
    uint32_t boneIndex;
    float boneWeight;
};
using bone_influences_per_control_point = std::vector<BoneInfluence>;

void fetch_bone_influences(const FbxMesh* fbxMesh,
    std::vector<bone_influences_per_control_point>& boneInfluences)
{
    const int controlPointsCount = fbxMesh->GetControlPointsCount();
    boneInfluences.resize(controlPointsCount);

    const int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int skinIndex = 0; skinIndex < skinCount; ++skinIndex)
    {
        const FbxSkin* fbxSkin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(skinIndex, FbxDeformer::eSkin));

        const int clusterCount = fbxSkin->GetClusterCount();
        for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
        {
            const FbxCluster* fbxCluster = fbxSkin->GetCluster(clusterIndex);

            const int controlPointIndicesCount = fbxCluster->GetControlPointIndicesCount();
            for (int controlPointIndicesIndex = 0; controlPointIndicesIndex < controlPointIndicesCount;
                ++controlPointIndicesIndex)
            {
                int controlPointIndex = fbxCluster->GetControlPointIndices()[controlPointIndicesIndex];
                double controlPointWeight = fbxCluster->GetControlPointWeights()[controlPointIndicesIndex];
                BoneInfluence& boneInfluence = boneInfluences.at(controlPointIndex).emplace_back();
                boneInfluence.boneIndex = static_cast<uint32_t>(clusterIndex);
                boneInfluence.boneWeight = static_cast<float>(controlPointWeight);
            }
        }
    }
}


SkinnedMesh::SkinnedMesh(ID3D11Device* device, const char* fbxFilename, bool triangulate,float samplingRate) {
    std::filesystem::path cerealFilename(fbxFilename);
    cerealFilename.replace_extension("cereal");
    if (std::filesystem::exists(cerealFilename.c_str())) {
        std::ifstream ifs(cerealFilename.c_str(),std::ios::binary);
        cereal::BinaryInputArchive deserialization(ifs);
        deserialization(sceneView, meshes, materials, animationClips);
    }
    else {

        FbxManager* fbxManager = FbxManager::Create();
        FbxScene* fbxScene = FbxScene::Create(fbxManager, "");

        FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
        bool importStatus = false;
        importStatus = fbxImporter->Initialize(fbxFilename);
        _ASSERT_EXPR_A(importStatus, fbxImporter->GetStatus().GetErrorString());

        importStatus = fbxImporter->Import(fbxScene);
        _ASSERT_EXPR_A(importStatus, fbxImporter->GetStatus().GetErrorString());

        FbxGeometryConverter fbxConverter = fbxManager;
        if (triangulate) {
            fbxConverter.Triangulate(fbxScene, true/*replace*/, false/*legacy*/);
            fbxConverter.RemoveBadPolygonsFromMeshes(fbxScene);
        }

        std::function<void(FbxNode*)> traverse = [&](FbxNode* fbxNode) {
            Scene::Node& node = sceneView.nodes.emplace_back();
            node.attribute = fbxNode->GetNodeAttribute() ?
                fbxNode->GetNodeAttribute()->GetAttributeType() : FbxNodeAttribute::EType::eUnknown;
            node.name = fbxNode->GetName();
            node.uniqueId = fbxNode->GetUniqueID();
            node.parentIndex = sceneView.indexof(fbxNode->GetParent() ?
                fbxNode->GetParent()->GetUniqueID() : 0);
            for (int childIndex = 0; childIndex < fbxNode->GetChildCount(); ++childIndex) {
                traverse(fbxNode->GetChild(childIndex));
            }
        };
        traverse(fbxScene->GetRootNode());

#if 0
        for (const Scene::Node& node : sceneView.nodes) {
            FbxNode* fbxNode = fbxScene->FindNodeByName(node.name.c_str());
            // Display node data in the output window as debug
            std::string nodeName = fbxNode->GetName();
            uint64_t uid = fbxNode->GetUniqueID();
            uint64_t parentUid = fbxNode->GetParent() ? fbxNode->GetParent()->GetUniqueID() : 0;
            int32_t type = fbxNode->GetNodeAttribute() ? fbxNode->GetNodeAttribute()->GetAttributeType() : 0;

            std::stringstream debugString;
            debugString << nodeName << ":" << uid << ":" << parentUid << ":" << type << "\n";
            OutputDebugStringA(debugString.str().c_str());
        }
#endif

        fetch_meshes(fbxScene, meshes);

        fetch_materials(fbxScene, materials);

        fetch_animations(fbxScene, animationClips, samplingRate);

        fbxManager->Destroy();

        std::ofstream ofs(cerealFilename.c_str(), std::ios::binary);
        cereal::BinaryOutputArchive serialization(ofs);
        serialization(sceneView, meshes, materials, animationClips);
    }

    create_com_objects(device, fbxFilename);
}

void SkinnedMesh::fetch_meshes(FbxScene* fbxScene, std::vector<Mesh>& meshes) {
    for (const Scene::Node& node : sceneView.nodes) {
        if (node.attribute != FbxNodeAttribute::EType::eMesh) {
            continue;
        }

        FbxNode* fbxNode = fbxScene->FindNodeByName(node.name.c_str());
        FbxMesh* fbxMesh = fbxNode->GetMesh();

        Mesh& mesh = meshes.emplace_back(); // C++17からemplace_backの戻り値がvoidじゃなく構築した要素への参照になっているためこの記述ができる
        mesh.uniqueId = fbxMesh->GetNode()->GetUniqueID();
        mesh.name = fbxMesh->GetNode()->GetName();
        mesh.nodeIndex = sceneView.indexof(mesh.uniqueId);
        mesh.defaultGlobalTransform = to_xmfloat4x4(fbxMesh->GetNode()->EvaluateGlobalTransform());

        std::vector<bone_influences_per_control_point> boneInfluences;
        fetch_bone_influences(fbxMesh, boneInfluences);
        fetch_skeleton(fbxMesh, mesh.bindPose);

        std::vector<Mesh::Subset>& subsets = mesh.subsets;
        const int materialCount = fbxMesh->GetNode()->GetMaterialCount();
        subsets.resize(materialCount > 0 ? materialCount : 1);
        for (int materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
            const FbxSurfaceMaterial* fbxMaterial = fbxMesh->GetNode()->GetMaterial(materialIndex);
            subsets.at(materialIndex).materialName = fbxMaterial->GetName();
            subsets.at(materialIndex).materialUniqueId = fbxMaterial->GetUniqueID();
        }
        if (materialCount > 0) {
            const int polygonCount = fbxMesh->GetPolygonCount();
            for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex) {
                const int materialIndex = fbxMesh->GetElementMaterial()->GetIndexArray().GetAt(polygonIndex);
                subsets.at(materialIndex).indexCount += 3;
            }
            uint32_t offset = 0;
            for (Mesh::Subset& subset : subsets) {
                subset.startIndexLocation = offset;
                offset += subset.indexCount;
                // This will be used as counter in the following procedures, reset to zero
                subset.indexCount = 0;
            }
        }

        const int polygonCount = fbxMesh->GetPolygonCount();
        mesh.vertices.resize(polygonCount * 3LL);
        mesh.indices.resize(polygonCount * 3LL);

        FbxStringList uvNames;
        fbxMesh->GetUVSetNames(uvNames);
        const FbxVector4* controlPoints = fbxMesh->GetControlPoints();
        for (int polygonIndex = 0; polygonIndex < polygonCount; ++polygonIndex) {
            const int materialIndex = materialCount > 0 ? fbxMesh->GetElementMaterial()->GetIndexArray().GetAt(polygonIndex) : 0;
            Mesh::Subset& subset = subsets.at(materialIndex);
            const uint32_t offset = subset.startIndexLocation + subset.indexCount;

            for (int positionInPolygon = 0; positionInPolygon < 3; ++positionInPolygon) {
                const int vertexIndex = polygonIndex * 3 + positionInPolygon;

                Vertex vertex;
                const int polygonVertex = fbxMesh->GetPolygonVertex(polygonIndex, positionInPolygon);
                vertex.position.x = static_cast<float>(controlPoints[polygonVertex][0]);
                vertex.position.y = static_cast<float>(controlPoints[polygonVertex][1]);
                vertex.position.z = static_cast<float>(controlPoints[polygonVertex][2]);

                const bone_influences_per_control_point& influencesPerControlPoint = boneInfluences.at(polygonVertex);
                for (size_t influenceIndex = 0; influenceIndex < influencesPerControlPoint.size(); ++influenceIndex) {
                    if (influenceIndex < MAX_BONE_INFLUENCES) {
                        vertex.boneWeights[influenceIndex] = influencesPerControlPoint.at(influenceIndex).boneWeight;
                        vertex.boneIndices[influenceIndex] = influencesPerControlPoint.at(influenceIndex).boneIndex;
                    }
                    // UNIT22の改善策はこれでいいのか？
                    else { 
                        // 影響度が最も小さいものを探し、削除
                        size_t minWeightIndex = 0;
                        float minWeight = vertex.boneWeights[0];

                        for (size_t i = 1; i < MAX_BONE_INFLUENCES; ++i) {
                            if (vertex.boneWeights[i] < minWeight) {
                                minWeight = vertex.boneWeights[i];
                                minWeightIndex = i;
                            }
                        }

                        // 影響度を削減
                        vertex.boneWeights[minWeightIndex] = influencesPerControlPoint.at(influenceIndex).boneWeight;
                        vertex.boneIndices[minWeightIndex] = influencesPerControlPoint.at(influenceIndex).boneIndex;
                    }
                }

                if (fbxMesh->GetElementNormalCount() > 0) {
                    FbxVector4 normal;
                    fbxMesh->GetPolygonVertexNormal(polygonIndex, positionInPolygon, normal);
                    vertex.normal.x = static_cast<float>(normal[0]);
                    vertex.normal.y = static_cast<float>(normal[1]);
                    vertex.normal.z = static_cast<float>(normal[2]);
                }
                if (fbxMesh->GetElementUVCount() > 0) {
                    FbxVector2 uv;
                    bool unmappedUv;
                    fbxMesh->GetPolygonVertexUV(polygonIndex, positionInPolygon,
                        uvNames[0], uv, unmappedUv);
                    vertex.texcoord.x = static_cast<float>(uv[0]);
                    vertex.texcoord.y = 1.0f - static_cast<float>(uv[1]);
                }
                if (fbxMesh->GenerateTangentsData(0, false)) {
                    const FbxGeometryElementTangent* tangent = fbxMesh->GetElementTangent(0); // Tangentは平面に沿った平行なベクトル Tangentは内積するときに使う
                    vertex.tangent.x = static_cast<float>(tangent->GetDirectArray().GetAt(vertexIndex)[0]);
                    vertex.tangent.y = static_cast<float>(tangent->GetDirectArray().GetAt(vertexIndex)[1]);
                    vertex.tangent.z = static_cast<float>(tangent->GetDirectArray().GetAt(vertexIndex)[2]);
                    vertex.tangent.w = static_cast<float>(tangent->GetDirectArray().GetAt(vertexIndex)[3]);
                }

                mesh.vertices.at(vertexIndex) = std::move(vertex);
                for (const Vertex& v : mesh.vertices) {
                    mesh.boundingBox[0].x = std::min<float>(mesh.boundingBox[0].x, v.position.x);
                    mesh.boundingBox[0].x = std::min<float>(mesh.boundingBox[0].y, v.position.y);
                    mesh.boundingBox[0].x = std::min<float>(mesh.boundingBox[0].z, v.position.z);
                    mesh.boundingBox[1].x = std::max<float>(mesh.boundingBox[1].x, v.position.x);
                    mesh.boundingBox[1].x = std::max<float>(mesh.boundingBox[1].y, v.position.y);
                    mesh.boundingBox[1].x = std::max<float>(mesh.boundingBox[1].z, v.position.z);
                }


                mesh.indices.at(static_cast<size_t>(offset) + positionInPolygon) = vertexIndex; // インデックスバッファーの特定の位置に頂点インデックスを設定
                subset.indexCount++;
            }
        }
    }
}

void SkinnedMesh::fetch_materials(FbxScene* fbxScene, std::unordered_map<uint64_t, Material>& materials) {
    const size_t nodeCount = sceneView.nodes.size();
    for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        const Scene::Node& node = sceneView.nodes.at(nodeIndex);
        const FbxNode* fbxNode = fbxScene->FindNodeByName(node.name.c_str());

        const int materialCount = fbxNode->GetMaterialCount();
        if (materialCount > 0) {
            for (int materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
                const FbxSurfaceMaterial* fbxMaterial = fbxNode->GetMaterial(materialIndex);

                Material material;
                material.name = fbxMaterial->GetName();
                material.uniqueId = fbxMaterial->GetUniqueID();
                FbxProperty fbxProperty;
                fbxProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);  // 陰のカラー情報を探す
                if (fbxProperty.IsValid()) {                                            // 見つけた情報が有効かどうか
                    const FbxDouble3 color = fbxProperty.Get<FbxDouble3>();             // 有効ならカラー情報の取得
                    material.Kd.x = static_cast<float>(color[0]);
                    material.Kd.y = static_cast<float>(color[1]);
                    material.Kd.z = static_cast<float>(color[2]);
                    material.Kd.w = 1.0f;

                    const FbxFileTexture* fbxTexture = fbxProperty.GetSrcObject<FbxFileTexture>(); // FbxFileTexture型のオブジェクトを入手
                    material.textureFilenames[0] =
                        fbxTexture ? fbxTexture->GetRelativeFileName() : ""; // 情報があればテクスチャファイルの相対パスを取得、無ければ空文字を代入
                }
                fbxProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sSpecular); // 反射光のカラー情報を探す
                if (fbxProperty.IsValid()) {                                            // 見つけた情報が有効かどうか
                    const FbxDouble3 color = fbxProperty.Get<FbxDouble3>();             // 有効ならカラー情報の取得
                    material.Ks.x = static_cast<float>(color[0]);
                    material.Ks.y = static_cast<float>(color[1]);
                    material.Ks.z = static_cast<float>(color[2]);
                    material.Ks.w = 1.0f;

                    const FbxFileTexture* fbxTexture = fbxProperty.GetSrcObject<FbxFileTexture>(); // FbxFileTexture型のオブジェクトを入手
                    material.textureFilenames[1] =
                        fbxTexture ? fbxTexture->GetRelativeFileName() : ""; // 情報があればテクスチャファイルの相対パスを取得、無ければ空文字を代入
                }
                fbxProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sAmbient);  // 環境光のカラー情報を探す
                if (fbxProperty.IsValid()) {                                            // 見つけた情報が有効かどうか
                    const FbxDouble3 color = fbxProperty.Get<FbxDouble3>();             // 有効ならカラー情報の取得
                    material.Ka.x = static_cast<float>(color[0]);
                    material.Ka.y = static_cast<float>(color[1]);
                    material.Ka.z = static_cast<float>(color[2]);
                    material.Ka.w = 1.0f;

                    const FbxFileTexture* fbxTexture = fbxProperty.GetSrcObject<FbxFileTexture>(); // FbxFileTexture型のオブジェクトを入手
                    material.textureFilenames[2] =
                        fbxTexture ? fbxTexture->GetRelativeFileName() : ""; // 情報があればテクスチャファイルの相対パスを取得、無ければ空文字を代入
                }
                fbxProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
                if (fbxProperty.IsValid()) {
                    const FbxFileTexture* fileTexture = fbxProperty.GetSrcObject<FbxFileTexture>();
                    material.textureFilenames[1] = fileTexture ? fileTexture->GetRelativeFileName() : "";
                }

                materials.emplace(material.uniqueId, std::move(material)); // mapコンテナなのでid値(中身を呼び出す鍵)とmaterial情報(中身)を追加
            }
        }
        else {
            Material material;
            material.name = "";
            material.uniqueId = 0;
            materials.emplace(material.uniqueId, std::move(material)); // ダミーマテリアルの生成
        }
    }
}

void SkinnedMesh::fetch_skeleton(FbxMesh* fbxMesh, Skeleton& bindPose) {
    const int deformerCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex) {
        FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
        const int clusterCount = skin->GetClusterCount(); // クラスタはスキンに作用する1本のボーンの情報を管理している
        bindPose.bones.resize(clusterCount);
        for (int clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex) {
            FbxCluster* cluster = skin->GetCluster(clusterIndex);

            Skeleton::Bone& bone = bindPose.bones.at(clusterIndex);
            bone.name = cluster->GetLink()->GetName();
            bone.uniqueId = cluster->GetLink()->GetUniqueID();
            bone.parentIndex = bindPose.indexof(cluster->GetLink()->GetParent()->GetUniqueID());
            bone.nodeIndex = sceneView.indexof(bone.uniqueId);

            //'referenceGlobalInitPosition' is used to convert from local space of model(mesh) to
            // global space of scene.
            FbxAMatrix referenceGlobalInitPosition;
            cluster->GetTransformMatrix(referenceGlobalInitPosition);

            // 'clusterGlobalInitPosition' is used to convert from local space of bone to
            // global space of scene.
            FbxAMatrix clusterGlobalInitPosition;
            cluster->GetTransformLinkMatrix(clusterGlobalInitPosition);

            // Matrices are defined using the Column Major scheme. When a FbxAMatrix represents a transformation
            // (translation, rotation and scale), the last row of the matrix represents the translation part of
            // the transformation.
            // Compose 'bone.offsetTransform' matrix that trnasforms position from mesh space to bone space.
            // This matrix is called the offset matrix.
            bone.offsetTransform = to_xmfloat4x4(clusterGlobalInitPosition.Inverse() * referenceGlobalInitPosition);
        }
    }
}

void SkinnedMesh::fetch_animations(FbxScene* fbxScene, std::vector<Animation>& animationClips,
    float samplingRate /* If this value is 0, the animation data will be sampling at the default frame rate. */) {
    FbxArray<FbxString*> animationStackNames;
    fbxScene->FillAnimStackNameArray(animationStackNames);
    const int animationStackCount = animationStackNames.GetCount();
    for (int animationStackIndex = 0; animationStackIndex < animationStackCount; ++animationStackIndex) {
        Animation& animationClip = animationClips.emplace_back();
        animationClip.name = animationStackNames[animationStackIndex]->Buffer();

        FbxAnimStack* animationStack = fbxScene->FindMember<FbxAnimStack>(animationClip.name.c_str());
        fbxScene->SetCurrentAnimationStack(animationStack);

        const FbxTime::EMode timeMode = fbxScene->GetGlobalSettings().GetTimeMode();
        FbxTime oneSecond;
        oneSecond.SetTime(0, 0, 1, 0, 0, timeMode);
        animationClip.samplingRate = samplingRate > 0 ?
            samplingRate : static_cast<float>(oneSecond.GetFrameRate(timeMode));

        const FbxTime samplingInterval = static_cast<FbxLongLong>(oneSecond.Get() / animationClip.samplingRate);
        const FbxTakeInfo* takeInfo = fbxScene->GetTakeInfo(animationClip.name.c_str()); // 指定したanimationClipの情報を取得的な処理多分
        const FbxTime startTime = takeInfo->mLocalTimeSpan.GetStart();
        const FbxTime stopTime = takeInfo->mLocalTimeSpan.GetStop();

        for (FbxTime time = startTime; time < stopTime; time += samplingInterval) {
            Animation::Keyframe& keyframe = animationClip.sequence.emplace_back();

            const size_t nodeCount = sceneView.nodes.size();
            keyframe.nodes.resize(nodeCount);

            for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
                FbxNode* fbxNode = fbxScene->FindNodeByName(sceneView.nodes.at(nodeIndex).name.c_str());
                if (fbxNode) {
                    Animation::Keyframe::Node& node = keyframe.nodes.at(nodeIndex);
                    // 'globalTransform' is a transformation matrix of anode with respect to
                    // the scene's global coordinate system.
                    node.globalTransform = to_xmfloat4x4(fbxNode->EvaluateGlobalTransform(time));

                    // 'localTransform' is a transformation matrix of a node with respect to
                    // its parent's local coordinate system.
                    const FbxAMatrix& localTransform = fbxNode->EvaluateLocalTransform(time);
                    node.scaling = to_xmfloat3(localTransform.GetS());
                    node.rotation = to_xmfloat4(localTransform.GetQ());
                    node.translation = to_xmfloat3(localTransform.GetT());
                }
            }
        }
    }
    for (int animationStackIndex = 0; animationStackIndex < animationStackCount; ++animationStackIndex) {
        delete animationStackNames[animationStackIndex];
    }
}

void SkinnedMesh::update_animation(Animation::Keyframe& keyframe) {
    size_t nodeCount = keyframe.nodes.size();
    for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        Animation::Keyframe::Node& node = keyframe.nodes.at(nodeIndex);
        XMMATRIX S = XMMatrixScaling(node.scaling.x, node.scaling.y, node.scaling.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&node.rotation));
        XMMATRIX T = XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);

        int64_t parentIndex = sceneView.nodes.at(nodeIndex).parentIndex;
        XMMATRIX P = parentIndex < 0 ? XMMatrixIdentity() :
            XMLoadFloat4x4(&keyframe.nodes.at(parentIndex).globalTransform);

        XMStoreFloat4x4(&node.globalTransform, S * R * T * P);
    }
}

bool SkinnedMesh::append_animations(const char* animationFilename, float samplingRate) {
    FbxManager* fbxManager = FbxManager::Create();
    FbxScene* fbxScene = FbxScene::Create(fbxManager, "");

    FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
    bool importStatus = false;
    importStatus = fbxImporter->Initialize(animationFilename);
    _ASSERT_EXPR_A(importStatus, fbxImporter->GetStatus().GetErrorString());

    importStatus = fbxImporter->Import(fbxScene);
    _ASSERT_EXPR_A(importStatus, fbxImporter->GetStatus().GetErrorString());

    fetch_animations(fbxScene, animationClips, samplingRate);

    fbxManager->Destroy();

    return true;
}

void SkinnedMesh::blend_animations(const Animation::Keyframe* keyframes[2], float factor,
    Animation::Keyframe& keyframe) {
    size_t nodeCount = keyframes[0]->nodes.size();
    keyframe.nodes.resize(nodeCount);
    for (size_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
        XMVECTOR S[2] = {
            XMLoadFloat3(&keyframes[0]->nodes.at(nodeIndex).scaling),
            XMLoadFloat3(&keyframes[1]->nodes.at(nodeIndex).scaling),
        };
        XMStoreFloat3(&keyframe.nodes.at(nodeIndex).scaling, XMVectorLerp(S[0],S[1],factor));

        XMVECTOR R[2] = {
            XMLoadFloat4(&keyframes[0]->nodes.at(nodeIndex).rotation),
            XMLoadFloat4(&keyframes[1]->nodes.at(nodeIndex).rotation),
        };
        XMStoreFloat4(&keyframe.nodes.at(nodeIndex).rotation, XMQuaternionSlerp(R[0], R[1], factor));

        XMVECTOR T[2] = {
            XMLoadFloat3(&keyframes[0]->nodes.at(nodeIndex).translation),
            XMLoadFloat3(&keyframes[1]->nodes.at(nodeIndex).translation),
        };
        XMStoreFloat3(&keyframe.nodes.at(nodeIndex).translation, XMVectorLerp(T[0], T[1], factor));
    }
}

void SkinnedMesh::create_com_objects(ID3D11Device* device, const char* fbxFilename) {
    for (Mesh& mesh : meshes) {
        HRESULT hr = S_OK;
        D3D11_BUFFER_DESC bufferDesc = {};
        D3D11_SUBRESOURCE_DATA subresourceData = {};
        bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size());
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;
        subresourceData.pSysMem = mesh.vertices.data();
        subresourceData.SysMemPitch = 0;
        subresourceData.SysMemSlicePitch = 0;
        hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.vertexBuffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

        bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        subresourceData.pSysMem = mesh.indices.data();
        hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.indexBuffer.ReleaseAndGetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
#if 1
        mesh.vertices.clear();
        mesh.indices.clear();
#endif
    }

    for (std::unordered_map<uint64_t, Material>::iterator iterator = materials.begin();
        iterator != materials.end(); ++iterator) {
        for (size_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
            if (iterator->second.textureFilenames[textureIndex].size() > 0) {
                std::filesystem::path path(fbxFilename);
                path.replace_filename(iterator->second.textureFilenames[textureIndex]);
                D3D11_TEXTURE2D_DESC texture2dDesc;
                load_texture_from_file(device, path.c_str(), iterator->second.shaderResouceViews[textureIndex].GetAddressOf(), &texture2dDesc);
            }
            else {
                make_dummy_texture(device, iterator->second.shaderResouceViews[textureIndex].GetAddressOf(),textureIndex == 1 ? 0xFFFF7F7F : 0xFFFFFFFF, 16);
            }
        }
    }


    HRESULT hr = S_OK;
    D3D11_INPUT_ELEMENT_DESC input_element_desc[]
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
        { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
        { "BONES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT },
    };
    create_vs_from_cso(device, "skinned_mesh_vs.cso", vertexShader.ReleaseAndGetAddressOf(),
    inputLayout.ReleaseAndGetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
    create_ps_from_cso(device, "skinned_mesh_ps.cso", pixelShader.ReleaseAndGetAddressOf());
    
     D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = sizeof(Constants);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = device->CreateBuffer(&buffer_desc, nullptr, constantBuffer.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void SkinnedMesh::render(ID3D11DeviceContext* immediateContext,
    const XMFLOAT4X4& world, const XMFLOAT4& materialColor,
    const Animation::Keyframe* keyframe) {
    for (const Mesh& mesh : meshes) {
        uint32_t stride = sizeof(Vertex);
        uint32_t offset = 0;
        immediateContext->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
        immediateContext->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        immediateContext->IASetInputLayout(inputLayout.Get());

        immediateContext->VSSetShader(vertexShader.Get(), nullptr, 0);
        immediateContext->PSSetShader(pixelShader.Get(), nullptr, 0);

        Constants data;

#if 0
        XMStoreFloat4x4(&data.boneTransforms[0], XMMatrixIdentity()); // 単位行列化
        XMStoreFloat4x4(&data.boneTransforms[1], XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(+45)));
        XMStoreFloat4x4(&data.boneTransforms[2], XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(-45)));
#endif

#if 0
        // Bind pose transform(Offset matrix) : Convert from the model(mesh) space to the bone space
        XMMATRIX B[3];
        B[0] = XMLoadFloat4x4(&mesh.bindPose.bones.at(0).offsetTransform); // 基準となる位置の設定
        B[1] = XMLoadFloat4x4(&mesh.bindPose.bones.at(1).offsetTransform); // 基準となる位置の設定
        B[2] = XMLoadFloat4x4(&mesh.bindPose.bones.at(2).offsetTransform); // 基準となる位置の設定

        // Animation bone transform : Convert from the bone space to the model(mesh) or the parent bone space
        XMMATRIX A[3];
        // from A0 space to model space
        A[0] = XMMatrixRotationRollPitchYaw(XMConvertToRadians(90), 0, 0);

        // from A1 space to parent bone(A0) space
        A[1] = XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(45)) * XMMatrixTranslation(0, 2, 0);

        // from A2 space to parent bone(A1) space
        A[2] = XMMatrixRotationRollPitchYaw(0, 0, XMConvertToRadians(-45)) * XMMatrixTranslation(0, 2, 0);
        
        // ボーンの計算
        XMStoreFloat4x4(&data.boneTransforms[0], B[0] * A[0]);
        XMStoreFloat4x4(&data.boneTransforms[1], B[1] * A[1] * A[0]);
        XMStoreFloat4x4(&data.boneTransforms[2], B[2] * A[2] * A[1] * A[0]);
#endif
        if (keyframe && keyframe->nodes.size() > 0) {
            const Animation::Keyframe::Node& meshNode = keyframe->nodes.at(mesh.nodeIndex);
            XMStoreFloat4x4(&data.world, XMLoadFloat4x4(&meshNode.globalTransform) * XMLoadFloat4x4(&world));
            const size_t boneCount = mesh.bindPose.bones.size();
            for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex) {
                const Skeleton::Bone& bone = mesh.bindPose.bones.at(boneIndex); // boneを指定
                const Animation::Keyframe::Node& boneNode = keyframe->nodes.at(bone.nodeIndex); // 指定したboneから情報を取得

                XMStoreFloat4x4(&data.boneTransforms[boneIndex],
                    XMLoadFloat4x4(&bone.offsetTransform) *                                 // 基準となる位置
                    XMLoadFloat4x4(&boneNode.globalTransform) *                             // 移動後の位置
                    XMMatrixInverse(nullptr, XMLoadFloat4x4(&mesh.defaultGlobalTransform))  // 基準となる位置の逆行列 ※nullptrを第一引数に入れることでポインタを格納せず計算でのみ使うことができる
                );
            }
        }
        else {
            XMStoreFloat4x4(&data.world,
                XMLoadFloat4x4(&mesh.defaultGlobalTransform) * XMLoadFloat4x4(&world));
            for (size_t boneIndex = 0; boneIndex < MAX_BONES; ++boneIndex) {
                data.boneTransforms[boneIndex] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
            }
        }
        for (const Mesh::Subset& subset : mesh.subsets) {
            const Material& material = materials.at(subset.materialUniqueId);

            XMStoreFloat4(&data.materialColor, XMLoadFloat4(&materialColor) * XMLoadFloat4(&material.Kd));

            immediateContext->UpdateSubresource(constantBuffer.Get(), 0, 0, &data, 0, 0);
            immediateContext->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

            immediateContext->PSSetShaderResources(0, 1, material.shaderResouceViews[0].GetAddressOf());
            immediateContext->PSSetShaderResources(1, 1, material.shaderResouceViews[1].GetAddressOf()); // specular用

            immediateContext->DrawIndexed(subset.indexCount, subset.startIndexLocation, 0);
        }
    }
}

inline XMFLOAT4X4 to_xmfloat4x4(const FbxAMatrix& fbxamatrix) {
    XMFLOAT4X4 xmfloat4x4;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            xmfloat4x4.m[row][column] = static_cast<float>(fbxamatrix[row][column]);
        }
    }
    return xmfloat4x4;
}

inline XMFLOAT3 to_xmfloat3(const FbxDouble3& fbxdouble3) {
    XMFLOAT3 xmfloat3;
    xmfloat3.x = static_cast<float>(fbxdouble3[0]);
    xmfloat3.y = static_cast<float>(fbxdouble3[1]);
    xmfloat3.z = static_cast<float>(fbxdouble3[2]);
    return xmfloat3;
}

inline XMFLOAT4 to_xmfloat4(const FbxDouble4& fbxdouble4) {
    XMFLOAT4 xmfloat4;
    xmfloat4.x = static_cast<float>(fbxdouble4[0]);
    xmfloat4.y = static_cast<float>(fbxdouble4[1]);
    xmfloat4.z = static_cast<float>(fbxdouble4[2]);
    xmfloat4.w = static_cast<float>(fbxdouble4[3]);
    return xmfloat4;
}