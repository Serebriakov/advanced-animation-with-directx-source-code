#include "MorphAnim.h"

// {2746B58A-B375-4cc3-8D23-7D094D3C7C67}
DEFINE_GUID(MorphAnimationKey,
            0x2746b58a, 0xb375, 0x4cc3, 
            0x8d, 0x23, 0x7d, 0x9, 0x4d, 0x3c, 0x7c, 0x67);

// {0892DE81-915A-4f34-B503-F7C397CB9E06}
DEFINE_GUID(MorphAnimationSet, 
            0x892de81, 0x915a, 0x4f34, 
            0xb5, 0x3, 0xf7, 0xc3, 0x97, 0xcb, 0x9e, 0x6);

cMorphAnimationKey::cMorphAnimationKey() 
{ 
  m_MeshName = NULL; 
  m_MeshPtr  = NULL; 
}

cMorphAnimationKey::~cMorphAnimationKey() 
{ 
  delete [] m_MeshName; m_MeshName = NULL;
  m_MeshPtr = NULL;
}

cMorphAnimationSet::cMorphAnimationSet()
{
  m_Name    = NULL;
  m_Length  = 0;
  m_Next    = NULL;
  m_NumKeys = 0;
  m_Keys    = NULL;
}

cMorphAnimationSet::~cMorphAnimationSet()
{
  delete [] m_Name; m_Name    = NULL;
  m_Length  = 0;
  m_NumKeys = 0;
  delete [] m_Keys; m_Keys    = NULL;
  delete m_Next;    m_Next    = NULL;
}

cMorphAnimationCollection::cMorphAnimationCollection()
{
  m_NumAnimationSets = 0;
  m_AnimationSets    = NULL;
}

cMorphAnimationCollection::~cMorphAnimationCollection()
{
  Free();
}

BOOL cMorphAnimationCollection::ParseObject(                  \
                    IDirectXFileData *pDataObj,               \
                    IDirectXFileData *pParentDataObj,         \
                    DWORD Depth,                              \
                    void **Data, BOOL Reference)
{
  const GUID *Type = GetObjectGUID(pDataObj);

  // Read in animation set data
  if(*Type == MorphAnimationSet) {

    // Create and link in a cMorphAnimationSet object
    cMorphAnimationSet *AnimSet = new cMorphAnimationSet();
    AnimSet->m_Next = m_AnimationSets;
    m_AnimationSets = AnimSet;

    // Increase # of animation sets
    m_NumAnimationSets++;

    // Set the animation set's name
    AnimSet->m_Name = GetObjectName(pDataObj);

    // Get data pointer
    DWORD *Ptr = (DWORD*)GetObjectData(pDataObj, NULL);

    // Get # of keys and allocate array of keyframe objects
    AnimSet->m_NumKeys = *Ptr++;
    AnimSet->m_Keys = new cMorphAnimationKey[AnimSet->m_NumKeys];

    // Get key data - time and mesh names
    for(DWORD i=0;i<AnimSet->m_NumKeys;i++) {
      AnimSet->m_Keys[i].m_Time     = *Ptr++;
      AnimSet->m_Keys[i].m_MeshName = strdup((char*)*Ptr++);
    }

    // Store length of animation
    AnimSet->m_Length = AnimSet->m_Keys[AnimSet->m_NumKeys-1].m_Time;
  }

  return ParseChildObjects(pDataObj, Depth, Data, Reference);
}

BOOL cMorphAnimationCollection::Load(char *Filename)
{
  // Free a prior collection
  Free();

  // Parse the file
  return Parse(Filename);
}

void cMorphAnimationCollection::Free()
{
  m_NumAnimationSets = 0;
  delete m_AnimationSets; m_AnimationSets = NULL;
}

void cMorphAnimationCollection::Map(D3DXMESHCONTAINER_EX *RootMesh)
{
  // Error checking
  if(!RootMesh)
    return;

  // Go through each animation set
  cMorphAnimationSet *AnimSet = m_AnimationSets;
  while(AnimSet != NULL) {

    // Go through each key in animation set and match
    // meshes to key's mesh pointers
    if(AnimSet->m_NumKeys) {
      for(DWORD i=0;i<AnimSet->m_NumKeys;i++)
        AnimSet->m_Keys[i].m_MeshPtr = RootMesh->Find(AnimSet->m_Keys[i].m_MeshName);
    }

    // Go to next animation set object
    AnimSet = AnimSet->m_Next;
  }
}

void cMorphAnimationCollection::Update(                       \
                            char *AnimationSetName,           \
                            DWORD Time, BOOL Loop,            \
                            D3DXMESHCONTAINER_EX **ppSource,  \
                            D3DXMESHCONTAINER_EX **ppTarget,  \
                            float *Scalar)
{
  cMorphAnimationSet *AnimSet = m_AnimationSets;

  // Clear targets
  *ppSource = NULL;
  *ppTarget = NULL;
  *Scalar = 0.0f;

  // Look for matching animation set name if used
  if(AnimationSetName) {

    // Find matching animation set name
    while(AnimSet != NULL) {

      // Break when match found
      if(!stricmp(AnimSet->m_Name, AnimationSetName))
        break;

      // Go to next animation set object
      AnimSet = AnimSet->m_Next;
    }
  }

  // Return no set found
  if(AnimSet == NULL)
    return;

  // Return if no keys in set
  if(!AnimSet->m_NumKeys)
    return;

  // Bounds time to animation lemgth
  if(Time > AnimSet->m_Length)
    Time = (Loop==TRUE)?Time%(AnimSet->m_Length+1):AnimSet->m_Length;

  // Go through animation set and look for keys to use
  DWORD Key1 = AnimSet->m_NumKeys-1;
  DWORD Key2 = AnimSet->m_NumKeys-1;
  for(DWORD i=0;i<AnimSet->m_NumKeys-1;i++) {
    if(Time >= AnimSet->m_Keys[i].m_Time &&                   \
       Time <  AnimSet->m_Keys[i+1].m_Time) {
       
      // Found the key, set pointers and break
      Key1 = i;
      Key2 = i+1;
      break;
    }
  }

  // Calculate a tiem scalar value to use
  DWORD Key1Time = AnimSet->m_Keys[Key1].m_Time;
  DWORD Key2Time = AnimSet->m_Keys[Key2].m_Time;
  float KeyTime = (float)(Time - Key1Time);
  float MorphScale = 1.0f/(float)(Key2Time-Key1Time)*KeyTime;

  // Set pointers
  *ppSource = AnimSet->m_Keys[Key1].m_MeshPtr;
  *ppTarget = AnimSet->m_Keys[Key2].m_MeshPtr;
  *Scalar   = MorphScale;
}
