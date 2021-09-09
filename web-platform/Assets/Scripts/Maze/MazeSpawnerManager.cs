using UnityEngine;
using System.Collections.Generic;

public class MazeSpawnerManager : Singleton<MazeSpawnerManager>
{
    [Header("Generation Settings")]
    public MazeGenerationAlgorithm Algorithm = MazeGenerationAlgorithm.RecursiveTree;
    public bool FullRandom = true;
    public int RandomSeed = 12345;

    [Header("Maze Settings")]
    public int Rows = 5;
    public int Columns = 5;
    public float CellSize = 4;
    public bool AddGaps = true;

    [Header("Maze Prefabs")]
    public GameObject MazeHolder = null;
    public GameObject PilarHolder = null;
    public GameObject FloorPrefab = null;
    public GameObject WallPrefab = null;
    public GameObject PillarPrefab = null;
    public List<GameObject> CollectablePrefabs = new List<GameObject>();

    public enum MazeGenerationAlgorithm { RecursiveTree, RandomTree, OldestTree }

    [HideInInspector] public BasicMazeGenerator mMazeGenerator = null;

    private void Start()
    {
        SetAlgorithm();
        PlacePrefabs();
    }

    private void SetAlgorithm()
    {
        if (!FullRandom) Random.InitState(RandomSeed);

        switch (Algorithm)
        {

            case MazeGenerationAlgorithm.RecursiveTree:
                mMazeGenerator = new RecursiveTreeMazeGenerator(Rows, Columns);
                break;
            case MazeGenerationAlgorithm.RandomTree:
                mMazeGenerator = new RandomTreeMazeGenerator(Rows, Columns);
                break;
            case MazeGenerationAlgorithm.OldestTree:
                mMazeGenerator = new OldestTreeMazeGenerator(Rows, Columns);
                break;

        }
    }

    private void PlacePrefabs()
    {
        mMazeGenerator.GenerateMaze();
        for (int row = 0; row < Rows; row++)
        {
            for (int column = 0; column < Columns; column++)
            {
                float x = column * (CellSize + (AddGaps ? .2f : 0));
                float z = row * (CellSize + (AddGaps ? .2f : 0));
                MazeCell cell = mMazeGenerator.GetMazeCell(row, column);

                GameObject floor = SpawnPrefab(FloorPrefab, new Vector3(x, 0, z), 0, MazeHolder);

                if (cell.WallFront) SpawnPrefab(WallPrefab, new Vector3(x, 0, z + CellSize / 2), 0, floor);
                if (cell.WallRight) SpawnPrefab(WallPrefab, new Vector3(x + CellSize / 2, 0, z), 90, floor);
                if (cell.WallBack) SpawnPrefab(WallPrefab, new Vector3(x, 0, z - CellSize / 2), 180, floor);
                if (cell.WallLeft) SpawnPrefab(WallPrefab, new Vector3(x - CellSize / 2, 0, z), 270, floor);
                if (cell.IsGoal && CollectablePrefabs != null && CollectablePrefabs.Count > 0) SpawnPrefab(CollectablePrefabs[Random.Range(0, CollectablePrefabs.Count - 1)], new Vector3(x, 1, z), 0, floor);
            }
        }

        if (PillarPrefab != null)
        {
            for (int row = 0; row < Rows + 1; row++)
            {
                for (int column = 0; column < Columns + 1; column++)
                {
                    float x = column * (CellSize + (AddGaps ? .2f : 0));
                    float z = row * (CellSize + (AddGaps ? .2f : 0));
                    SpawnPrefab(PillarPrefab, new Vector3(x - CellSize / 2, 0, z - CellSize / 2), 0, PilarHolder);
                }
            }
        }
    }

    private GameObject SpawnPrefab(GameObject prefab, Vector3 position, float yRotation, GameObject parent)
    {
        GameObject spawned = Instantiate(prefab, prefab.transform.position + position, Quaternion.Euler(0, yRotation, 0)) as GameObject;
        spawned.transform.parent = parent.transform;
        return spawned;
    }
}
