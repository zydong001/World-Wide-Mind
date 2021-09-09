using UnityEngine;
using System.Collections;

public class MovementScript : MonoBehaviour
{
    public float rotateSpeed = 0.8f;
    public float walkSpeed = 0.8f;

    private MazeSpawnerManager mazeSpawnerManager;
    private bool isRotating = false;
    private bool isMoving = false;

    private void Start() => mazeSpawnerManager = MazeSpawnerManager.Instance;

    private void Update()
    {
        if (!isRotating && !isMoving)
        {
            Rotating();
            Moving();
        }
    }

    private void Rotating()
    {
        bool left = Input.GetKeyDown(KeyCode.A) || Input.GetKeyDown(KeyCode.LeftArrow);
        bool right = Input.GetKeyDown(KeyCode.D) || Input.GetKeyDown(KeyCode.RightArrow);

        if (right) StartCoroutine(SmoothRotate(Vector3.up * 90, rotateSpeed));
        else if (left) StartCoroutine(SmoothRotate(Vector3.up * -90, rotateSpeed));
    }

    private void Moving()
    {
        bool forward = Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.UpArrow);
        bool isWall = Physics.Raycast(transform.position, transform.forward, mazeSpawnerManager.CellSize);

        if (forward && !isWall) StartCoroutine(SmoothMove(transform.forward * mazeSpawnerManager.CellSize, walkSpeed));
    }

    private IEnumerator SmoothRotate(Vector3 byAngles, float inTime)
    {
        isRotating = true;
        Quaternion fromAngle = transform.rotation;
        Quaternion toAngle = Quaternion.Euler(transform.eulerAngles + byAngles);
        for (float t = 0f; t < 1; t += Time.deltaTime / inTime)
        {
            transform.rotation = Quaternion.Slerp(fromAngle, toAngle, t);
            yield return null;
        }
        transform.rotation = toAngle;
        isRotating = false;
    }

    private IEnumerator SmoothMove(Vector3 target, float inTime)
    {
        isMoving = true;
        Vector3 fromPos = transform.position;
        Vector3 toPos = fromPos + target;
        for (var t = 0f; t < 1; t += Time.deltaTime / inTime)
        {
            transform.position = Vector3.Lerp(fromPos, toPos, t);
            yield return null;
        }
        transform.position = toPos;
        isMoving = false;
    }
}
